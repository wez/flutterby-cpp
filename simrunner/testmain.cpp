
// Replace exits with some code that sleeps with interrupts
// disabled; this causes the simulator to exit
extern "C" void exit(int result) {
  if (result != 0) {
    // Generate an illegal access to trigger a crash instead
    *((char*)(0xffff - result)) = 42;
  }
  __builtin_avr_cli();
  __builtin_avr_sleep();
  for(;;);
}
