MCU=atmega32u4
CXXFLAGS=-std=c++17 -fno-exceptions -gstabs -mmcu=$(MCU) -Os -DHAVE_SIMAVR=1

all: rust-deps target/blink.elf

rust-deps:
	cargo run -p atdf2cpp

target/blink.elf: examples/blink.cpp include/flutterby/bitflags.h target/atmega32u4.h
	mkdir -p target
	avr-g++ $(CXXFLAGS) -o $@ -Itarget -Iinclude -I/usr/local/include $<

clean:
	rm -rf target
