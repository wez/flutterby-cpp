#include <simavr/sim_avr.h>
#include <simavr/sim_elf.h>
#include <stdio.h>

// This code runs an avr simulator in almost the same way as the
// simavr runner from the main distro.  The biggest difference is
// that we terminate with exit code 1 when the firmware crashes,
// rather than triggering gdb.

int main(int argc, char** argv) {
  auto filename = argv[1];

  elf_firmware_t f = {{0}};

  if (elf_read_firmware(filename, &f)) {
    fprintf(stderr, "failed to load firmware %s\n", filename);
    return 1;
  }

  auto avr = avr_make_mcu_by_name(f.mmcu);
  avr_init(avr);
  avr_load_firmware(avr, &f);

  int state;
  for (;;) {
    state = avr_run(avr);
    if (state == cpu_Done || state == cpu_Crashed)
      break;
  }

  avr_terminate(avr);
  return state == cpu_Crashed;
}
