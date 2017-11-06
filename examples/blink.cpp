#include "atmega32u4.h"
using namespace flutterby;

int main() {
  /*
  auto mcusr = Cpu::mcusr | CpuMcusrFlags::WDRF;
  auto other = mcusr ;//| CpuMcusrFlags::WDRF;
  Cpu::mcusr = other;
  Cpu::mcusr |= CpuMcusrFlags::WDRF - CpuMcusrFlags::BORF;
  */

  Portd::pind |= PortdSignalFlags::PD0;

  return Cpu::mcusr.raw_bits();
}
