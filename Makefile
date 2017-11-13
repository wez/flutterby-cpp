#MCU=atmega32u4
#F_CPU=8000000

MCU?=atmega328p
F_CPU?=16000000

ifeq (1,${DEBUG})
DEBUG_ENABLE=-DHAVE_SIMAVR=1 -Wl,--section-start=.mmcu=0x910000
TDIR=target/$(MCU)-sim
else
TDIR=target/$(MCU)
endif

AVR_CXXFLAGS=-std=c++17 -fno-exceptions -g -mmcu=$(MCU) -MMD -MF $@.d -MP -Wa,-adln=$@.s -fverbose-asm -Os $(DEBUG_ENABLE) -DF_CPU=$(F_CPU) -I$(TDIR) -Iinclude -I/usr/local/include

LIBSRCS=$(wildcard lib/*.cpp)
LIBOBJS=$(patsubst lib/%,$(TDIR)/lib/%,$(LIBSRCS:.cpp=.o))

EXAMPLESRCS=$(wildcard examples/*.cpp)
EXAMPLEOBJS=$(patsubst examples/%,$(TDIR)/examples/%,$(EXAMPLESRCS:.cpp=.o))
EXAMPLEEXE=$(EXAMPLEOBJS:.o=.elf)

TESTSRCS=$(wildcard tests/*.cpp)
TESTOBJS=$(patsubst tests/%,$(TDIR)/tests/%,$(TESTSRCS:.cpp=.o))
TESTEXE=$(TESTOBJS:.o=.elf)

all: ex

include $(shell find $(TDIR) -name \*.d)

target/debug/atdf2cpp: atdf2cpp/src/main.rs
	cargo build -p atdf2cpp

$(TDIR)/avr_autogen.h: target/debug/atdf2cpp
	@mkdir -p $(@D)
	cargo run -p atdf2cpp $(MCU) $@

target/simrunner: simrunner/main.cpp
	@mkdir -p $(@D)
	$(CXX) -std=c++11 -o $@ -lsimavr -lelf $<

$(TDIR)/lib/%.o: lib/%.cpp $(TDIR)/avr_autogen.h
	@mkdir -p $(@D)
	avr-g++ $(AVR_CXXFLAGS) -c -o $@ $<


.PHONY: ex
ex: $(EXAMPLEEXE)

$(TDIR)/tests/%.o: tests/%.cpp $(TDIR)/avr_autogen.h
	@mkdir -p $(@D)
	avr-g++ $(AVR_CXXFLAGS) -c -o $@ $<

$(TDIR)/tests/%.elf: $(TDIR)/tests/%.o $(TDIR)/testmain.o $(TDIR)/lib$(MCU).a
	@mkdir -p $(@D)
	avr-g++ $(AVR_CXXFLAGS) -o $@ -L$(TDIR) -l$(MCU) $(TDIR)/testmain.o $<

$(TDIR)/examples/%.o: examples/%.cpp $(TDIR)/avr_autogen.h
	@mkdir -p $(@D)
	avr-g++ $(AVR_CXXFLAGS) -c -o $@ $<

$(TDIR)/examples/%.elf: $(TDIR)/examples/%.o $(TDIR)/lib$(MCU).a
	@mkdir -p $(@D)
	avr-g++ $(AVR_CXXFLAGS) -o $@ -L$(TDIR) -l$(MCU) $<
	avr-size --format=avr --mcu=$(MCU) $@

$(TDIR)/lib$(MCU).a: $(LIBOBJS)
	avr-ar rcu $@ $(LIBOBJS)

#$(TDIR)/blink.elf: examples/blink.cpp include/flutterby/bitflags.h $(TDIR)/avr_autogen.h target/lib$(MCU).a
#	@mkdir -p $(@D)
#	avr-g++ $(AVR_CXXFLAGS) -o $@ $< -Ltarget -l$(MCU)
#	avr-size --format=avr --mcu=$(MCU) $@

sim: $(TDIR)/examples/blink.elf target/simrunner
	@mkdir -p $(@D)
	@echo avr-gdb $(TDIR)/examples/blink.elf -ex \"target remote :1234\" -tui
	target/simrunner $(TDIR)/examples/blink.elf

$(TDIR)/testmain.o: simrunner/testmain.cpp
	@mkdir -p $(@D)
	avr-g++ $(AVR_CXXFLAGS) -c -o $@ $<

.PHONY: t
t: target/simrunner $(TESTEXE)
	for t in $(TESTEXE) ; do target/simrunner $$t && echo "OK: $$t" || exit 1 ; done

#simavr -m $(MCU) -f $(F_CPU) -v -v -v -v -v target/blink.elf

clean:
	rm -rf target
