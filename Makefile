MCU=atmega32u4
F_CPU=8000000
AVR_CXXFLAGS=-std=c++17 -fno-exceptions -g -mmcu=$(MCU) -Wa,-adln=target/wat.s -fverbose-asm -Os -DHAVE_SIMAVR=1 -DF_CPU=$(F_CPU) -Wl,--section-start=.mmcu=0x910000 -Itarget -Iinclude -I/usr/local/include

all: rust-deps target/blink.elf

rust-deps:
	cargo run -p atdf2cpp

target/simrunner: simrunner/main.cpp
	$(CXX) -std=c++11 -o $@ -lsimavr -lelf $<

LIBSRCS=$(wildcard lib/*.cpp)
LIBOBJS=$(LIBSRCS:.cpp=.o)

lib/%.o: lib/%.cpp
	avr-g++ $(AVR_CXXFLAGS) -c -o $@ $<

target/lib$(MCU).a: $(LIBOBJS)
	avr-ar rcu $@ $^

target/blink.elf: examples/blink.cpp include/flutterby/bitflags.h target/atmega32u4.h target/lib$(MCU).o
	mkdir -p target
	avr-g++ $(AVR_CXXFLAGS) -o $@ $< -Ltarget -l$(MCU) simrunner/testmain.cpp
	avr-size --format=avr --mcu=$(MCU) $@

sim: target/blink.elf target/simrunner
	@echo avr-gdb target/blink.elf -ex \"target remote :1234\" -tui
	target/simrunner target/blink.elf

target/testmain.o: simrunner/testmain.cpp
	avr-g++ $(AVR_CXXFLAGS) -c -o $@ $<

.PHONY: t
t: target/simrunner target/testmain.o target/lib$(MCU).a
	mkdir -p target/tests
	for t in tests/*.cpp ; do  avr-g++ $(AVR_CXXFLAGS) -Ltarget -l$(MCU) -o target/$$t.elf $$t target/testmain.o && target/simrunner target/$$t.elf || exit 1 ; done

#simavr -m $(MCU) -f $(F_CPU) -v -v -v -v -v target/blink.elf

clean:
	rm -rf target
