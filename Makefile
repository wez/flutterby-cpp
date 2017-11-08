MCU=atmega32u4
F_CPU=8000000
CXXFLAGS=-std=c++17 -fno-exceptions -g -mmcu=$(MCU) -Wa,-adln=target/wat.s -fverbose-asm -Os -DHAVE_SIMAVR=1 -DF_CPU=$(F_CPU) -Wl,--section-start=.mmcu=0x910000

all: rust-deps target/blink.elf

rust-deps:
	cargo run -p atdf2cpp

target/blink.elf: examples/blink.cpp include/flutterby/bitflags.h target/atmega32u4.h
	mkdir -p target
	avr-g++ $(CXXFLAGS) -o $@ -Itarget -Iinclude -I/usr/local/include $<
	avr-size --format=avr --mcu=$(MCU) $@

sim: target/blink.elf
	@echo avr-gdb target/blink.elf -ex \"target remote :1234\" -tui
	simavr -m $(MCU) -f $(F_CPU) -v -v -v -v -v target/blink.elf

clean:
	rm -rf target
