#MCU=atmega32u4
#F_CPU=8000000

MCU=atmega328p
F_CPU=16000000

AVR_CXXFLAGS=-std=c++17 -fno-exceptions -g -mmcu=$(MCU) -Wa,-adln=$@.s -fverbose-asm -Os -DHAVE_SIMAVR=1 -DF_CPU=$(F_CPU) -Wl,--section-start=.mmcu=0x910000 -Itarget/$(MCU) -Iinclude -I/usr/local/include

all: target/$(MCU)/blink.elf

target/debug/atdf2cpp: atdf2cpp/src/main.rs
	@mkdir -p $(@D) target/$(MCU)
	cargo run -p atdf2cpp $(MCU)

target/$(MCU)/avr_autogen.h: target/debug/atdf2cpp

target/simrunner: simrunner/main.cpp
	@mkdir -p $(@D)
	$(CXX) -std=c++11 -o $@ -lsimavr -lelf $<

LIBSRCS=$(wildcard lib/*.cpp)
LIBOBJS=$(patsubst lib/%,target/lib$(MCU)/%,$(LIBSRCS:.cpp=.o))

target/lib$(MCU)/%.o: lib/%.cpp target/$(MCU)/avr_autogen.h
	@mkdir -p $(@D)
	avr-g++ $(AVR_CXXFLAGS) -c -o $@ $<

TESTSRCS=$(wildcard tests/*.cpp)
TESTOBJS=$(patsubst tests/%,target/$(MCU)/tests/%,$(TESTSRCS:.cpp=.o))
TESTEXE=$(TESTOBJS:.o=.elf)

target/$(MCU)/tests/%.o: tests/%.cpp target/$(MCU)/avr_autogen.h
	@mkdir -p $(@D)
	avr-g++ $(AVR_CXXFLAGS) -c -o $@ $<

target/$(MCU)/tests/%.elf: tests/%.cpp target/$(MCU)/avr_autogen.h target/$(MCU)/testmain.o target/lib$(MCU).a
	@mkdir -p $(@D)
	avr-g++ $(AVR_CXXFLAGS) -o $@ -Ltarget -l$(MCU) target/$(MCU)/testmain.o $<

target/lib$(MCU).a: $(LIBOBJS)
	avr-ar rcu $@ $(LIBOBJS)

target/$(MCU)/blink.elf: examples/blink.cpp include/flutterby/bitflags.h target/$(MCU)/avr_autogen.h target/lib$(MCU).a target/$(MCU)/testmain.o
	@mkdir -p $(@D)
	avr-g++ $(AVR_CXXFLAGS) -o $@ $< -Ltarget -l$(MCU) target/$(MCU)/testmain.o
	avr-size --format=avr --mcu=$(MCU) $@

sim: target/blink.elf target/simrunner
	@mkdir -p $(@D)
	@echo avr-gdb target/blink.elf -ex \"target remote :1234\" -tui
	target/simrunner target/blink.elf

target/$(MCU)/testmain.o: simrunner/testmain.cpp
	@mkdir -p $(@D)
	avr-g++ $(AVR_CXXFLAGS) -c -o $@ $<

.PHONY: t
t: target/simrunner $(TESTEXE)
	for t in $(TESTEXE) ; do target/simrunner $$t && echo "OK: $$t" || exit 1 ; done

#simavr -m $(MCU) -f $(F_CPU) -v -v -v -v -v target/blink.elf

clean:
	rm -rf target
