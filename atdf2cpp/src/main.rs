#![feature(ascii_ctype)]

extern crate avr_mcu;
extern crate ident_case;

use std::env;
use std::fs::File;
use std::io::Write;
use std::ascii::AsciiExt;
use std::collections::{HashMap, HashSet};

use ident_case::RenameRule;

fn or_name<'a>(caption: &'a String, name: &'a String) -> &'a String {
    if caption.len() == 0 {
        name
    } else {
        caption
    }
}

/// "ADC Noise Reduction (If Available)" -> "ADC_Noise_Reduction_If_Available"
fn caption_to_ident(caption: &String, group_name: Option<&String>) -> String {
    let ident: String = caption
        .chars()
        .filter_map(|c| if c.is_ascii_whitespace() {
            Some('_')
        } else if c.is_ascii_alphanumeric() {
            Some(c)
        } else {
            None
        })
        .collect();

    let ident = if let Some(group_name) = group_name {
        format!("{}_{}", group_name, ident)
    } else {
        ident
    };


    if ident.as_bytes()[0].is_ascii_digit() {
        format!("_{}", ident)
    } else {
        ident
    }
}

fn main() {
    let name = env::args().nth(1).expect("name of mcu as first argument");
    let mcu = avr_mcu::microcontroller(&name);
    genmcu(&mcu, &name).expect("failed to generate mcu data");
}

fn placement(addr: u32) -> Option<&'static str> {
    if addr < 0x20 {
        // Not in the valid IO space range
        None
    } else if addr < 0x60 {
        // io
        Some("io")
    } else {
        // extended io
        Some("address")
    }
}

fn genmcu(mcu: &avr_mcu::Mcu, name: &str) -> std::io::Result<()> {
    let mut mcu_def = File::create(format!("target/{}/avr_autogen.h", name))?;

    writeln!(mcu_def, "#pragma once")?;
    writeln!(mcu_def, "#include <stdint.h>")?;
    writeln!(mcu_def, "#include <flutterby/Bitflags.h>")?;
    writeln!(mcu_def, "namespace flutterby {{")?;
    writeln!(mcu_def, "// MCU defs for {}", mcu.device.name)?;

    // Interrupt handler definitions.  These assume that we're linking
    // with the avr libc startup and that it will take care of putting
    // these specially named functions into the interrupt vector table.
    writeln!(mcu_def, "/// Helper for declaring IRQ handlers.")?;
    for interrupt in mcu.device.interrupts.iter() {
        writeln!(mcu_def, "#define IRQ_{} \\", interrupt.name)?;
        writeln!(
            mcu_def,
            "    extern \"C\" void __vector_{}(void) \\",
            interrupt.index
        )?;
        writeln!(
            mcu_def,
            "    __attribute__((signal,used,externally_visible)); \\"
        )?;
        writeln!(mcu_def, "    void __vector_{}(void)", interrupt.index)?;
    }

    let mut defined_signal_flags = HashSet::new();

    let mut instance_by_name = HashMap::new();
    for p in mcu.device.peripherals.iter() {
        for inst in p.instances.iter() {
            // Emit a cfg flag that we can test in the handwritten
            // code to tell whether a given peripheral instance
            // is available in the handwritten code.
            writeln!(mcu_def, "#define HAVE_AVR_{}", inst.name)?;
            instance_by_name.insert(&inst.name, inst);
        }
    }

    // Candidate locations for simavr registers
    let mut simavr_console_reg = None;
    let mut simavr_command_reg = None;

    // Emit registers
    for module in mcu.modules.iter() {
        writeln!(mcu_def, "/// Registers for {}", mcu.device.name)?;

        let mut value_group_by_name = HashMap::new();
        for vg in module.value_groups.iter() {
            value_group_by_name.insert(&vg.name, vg);
        }

        for group in module.register_groups.iter() {
            writeln!(mcu_def, "")?;
            writeln!(mcu_def, "/// Register group: {}", group.name)?;
            writeln!(mcu_def, "/// {}", group.caption)?;

            let struct_name =
                RenameRule::PascalCase.apply_to_field(group.name.to_ascii_lowercase());

            let mut sorted_regs = group.registers.clone();
            sorted_regs.sort_by(|a, b| a.offset.cmp(&b.offset));

            let mut have_reg_type = HashMap::new();


            // First pass through to define some bitfields
            for reg in sorted_regs.iter() {
                let flag_reg_name =
                    RenameRule::PascalCase.apply_to_field(reg.name.to_ascii_lowercase());
                let mut flags_name = format!("{}{}Flags", struct_name, flag_reg_name);
                let register_type = if reg.size == 1 { "uint8_t" } else { "uint16_t" };

                let mut bitfields = Vec::new();
                let mut done_value_name: HashSet<String> = HashSet::new();

                for field in reg.bitfields.iter() {
                    if let Some(ref value_group) = field.values {
                        // References a handy precompute mask.  The labels for these are
                        // a bit hit and miss, so we generate two versions; one using the
                        // name and one using the caption text so that we double the chances
                        // of getting a meaningful result.
                        let vg = value_group_by_name.get(value_group).expect(&format!(
                            "broken value group linkage from field {:?}",
                            field
                        ));

                        for value in vg.values.iter() {
                            let name = caption_to_ident(&value.caption, Some(&field.name));
                            if done_value_name.contains(&name) {
                                continue;
                            }
                            done_value_name.insert(name.clone());

                            // We need to modify the value from the group to match the mask
                            // on the current field.  We do this by shifting off bits from the
                            // mask until we no longer have zero bits on the LSB end.  The
                            // number of bits shifted off is the number of bits we need to
                            // shift the value group value in the opposite direction.
                            let mut vg_value = value.value;
                            let mut mask = field.mask;
                            while mask & 1 == 0 {
                                mask = mask >> 1;
                                vg_value = vg_value << 1;
                            }

                            bitfields.push((name, vg_value, or_name(&value.caption, &value.name)));
                        }
                    } else {
                        bitfields.push((
                            field.name.clone(),
                            field.mask,
                            or_name(&field.caption, &field.name),
                        ));
                    }
                }
                if bitfields.len() == 0 {
                    // Let's see if we can find matching peripheral data
                    flags_name = format!(
                        "{}SignalFlags",
                        RenameRule::PascalCase.apply_to_field(group.name.to_ascii_lowercase())
                    );
                    if defined_signal_flags.contains(&flags_name) {
                        // we already defined this struct
                        have_reg_type.insert(&reg.name, flags_name.clone());
                        continue;
                    }
                    if let Some(inst) = instance_by_name.get(&group.name) {
                        for sig in inst.signals.iter() {
                            if let Some(bitno) = sig.index {
                                bitfields.push((sig.pad.clone(), 1 << bitno, &sig.pad));

                                defined_signal_flags.insert(flags_name.clone());
                            }
                        }
                    }
                }

                if bitfields.len() == 0 {
                    continue;
                }

                have_reg_type.insert(&reg.name, flags_name.clone());

                writeln!(mcu_def, "namespace {} {{", flags_name)?;
                writeln!(mcu_def, "enum class {}: {} {{", flags_name, register_type)?;
                //writeln!(mcu_def, "enum {} {{", flags_name)?;
                for &(ref name, mask, caption) in bitfields.iter() {
                    writeln!(mcu_def, "    /// {}", caption)?;
                    writeln!(
                        mcu_def,
                        "    {} = {},",
                        RenameRule::ScreamingSnakeCase.apply_to_field(&name),
                        mask
                    )?;
                }
                writeln!(mcu_def, "}};")?;
                for (name, _mask, _caption) in bitfields {
                    writeln!(
                        mcu_def,
                        "static const constexpr bitflags<{}, {}> {}({}::{});",
                        flags_name,
                        register_type,
                        RenameRule::ScreamingSnakeCase.apply_to_field(&name),
                        flags_name,
                        RenameRule::ScreamingSnakeCase.apply_to_field(&name),
                    )?;
                }
                writeln!(mcu_def, "}};")?;
            }

            writeln!(mcu_def, "namespace {} {{", struct_name)?;

            for idx in 0..sorted_regs.len() {
                // do we need a hole?
                let register = &sorted_regs[idx];

                if idx > 0 {
                    let prior = &sorted_regs[idx - 1];

                    let hole_size = register.offset - (prior.offset + prior.size);
                    if hole_size > 0 {
                        // Can we stick the simavr special regs in this hole?
                        let mut reg_space = hole_size;
                        let mut reg_addr = prior.offset + prior.size;
                        if simavr_console_reg.is_none() {
                            simavr_console_reg = Some(reg_addr);
                            reg_space -= 1;
                            reg_addr += 1;
                        }

                        if simavr_command_reg.is_none() && reg_space > 0 {
                            simavr_command_reg = Some(reg_addr);
                        }
                    }
                }

                let reg_name =
                    RenameRule::SnakeCase.apply_to_field(register.name.to_ascii_lowercase());
                let register_size_type = if register.size == 1 {
                    "uint8_t".to_owned()
                } else {
                    "uint16_t".to_owned()
                };
                let register_type = match have_reg_type.get(&register.name) {
                    Some(name) => format!(
                        "bitflags<enum {}::{}, {}>",
                        name.to_owned(),
                        name.to_owned(),
                        register_size_type
                    ),
                    _ => register_size_type,
                };


                if let Some(placement) = placement(register.offset) {
                    writeln!(mcu_def, "/// {}", register.name)?;
                    writeln!(
                        mcu_def,
                        "static volatile {} {} __attribute__(({}({})));",
                        register_type,
                        reg_name,
                        placement,
                        register.offset,
                    )?;
                }
            }

            writeln!(mcu_def, "}}")?;
        }
    }

    writeln!(mcu_def, "\n\n")?;

    writeln!(mcu_def, "#ifdef HAVE_SIMAVR")?;
    writeln!(mcu_def, "#include <simavr/avr/avr_mcu_section.h>")?;
    writeln!(mcu_def, "__attribute__((used))")?;
    writeln!(mcu_def, "AVR_MCU_LONG(AVR_MMCU_TAG_FREQUENCY, F_CPU);")?;
    writeln!(mcu_def, "__attribute__((used))")?;
    writeln!(
        mcu_def,
        "const struct avr_mmcu_string_t _mmu_name _MMCU_ = {{ \
         AVR_MMCU_TAG_NAME,\
         sizeof(struct avr_mmcu_string_t) - 2,\
         {{ \"{}\" }}\
         }};",
        mcu.device.name.to_ascii_lowercase()
    )?;
    if let Some(simavr_console_reg) = simavr_console_reg {
        writeln!(mcu_def, "/// Simavr console")?;
        writeln!(
            mcu_def,
            "static volatile uint8_t SIMAVR_CONSOLE \
             __attribute__(({}({})));",
            placement(simavr_console_reg).unwrap(),
            simavr_console_reg
        )?;

        writeln!(mcu_def, "__attribute__((used))")?;
        writeln!(mcu_def, "AVR_MCU_SIMAVR_CONSOLE({});", simavr_console_reg)?;
    }

    if let Some(simavr_command_reg) = simavr_command_reg {
        writeln!(mcu_def, "/// Simavr command")?;
        writeln!(
            mcu_def,
            "static volatile uint8_t SIMAVR_COMMAND \
             __attribute__(({}({})));",
            placement(simavr_command_reg).unwrap(),
            simavr_command_reg
        )?;
        writeln!(mcu_def, "__attribute__((used))")?;
        writeln!(mcu_def, "AVR_MCU_SIMAVR_COMMAND({});", simavr_command_reg)?;
    }

    writeln!(mcu_def, "#endif")?;
    writeln!(mcu_def, "}}")?;
    writeln!(mcu_def, "\n\n")?;

    Ok(())
}
