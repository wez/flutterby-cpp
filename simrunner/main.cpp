#include <simavr/sim_avr.h>
#include <simavr/sim_elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

const char *firmware_filename = nullptr;

// This code runs an avr simulator in almost the same way as the
// simavr runner from the main distro.  The biggest difference is
// that we terminate with exit code 1 when the firmware crashes,
// rather than triggering gdb.

static void logger(struct avr_t* avr, const int level, const char * format, va_list ap) {
  auto threshold = avr ? avr->log : LOG_ERROR;
  if (level > threshold) {
    return;
  }
  vfprintf((level > LOG_ERROR) ? stdout : stderr, format, ap);
}

static void
console_write(struct avr_t* avr, avr_io_addr_t addr, uint8_t v, void* param) {
  if (v == '\r' && avr->io_console_buffer.buf) {
    avr->io_console_buffer.buf[avr->io_console_buffer.len] = 0;
    AVR_LOG(
        avr,
        LOG_OUTPUT,
        "%ld: %s: %s\n",
        time(nullptr),
        firmware_filename,
        avr->io_console_buffer.buf);
    avr->io_console_buffer.len = 0;
    return;
  }
  if (avr->io_console_buffer.len + 1 >= avr->io_console_buffer.size) {
    avr->io_console_buffer.size += 128;
    avr->io_console_buffer.buf =
        (char*)realloc(avr->io_console_buffer.buf, avr->io_console_buffer.size);
  }
  if (v > 0) {
    avr->io_console_buffer.buf[avr->io_console_buffer.len++] = v;
  }
}

int main(int argc, char** argv) {
  firmware_filename = argv[1];

  elf_firmware_t f = {{0}};

  // Suppress firmware loading messages on the assumption that it will succeed
  avr_global_logger_set(logger);

  if (elf_read_firmware(firmware_filename, &f)) {
    fprintf(stderr, "failed to load firmware %s\n", firmware_filename);
    return 1;
  }

  auto avr = avr_make_mcu_by_name(f.mmcu);
  avr_init(avr);
  avr->log = LOG_OUTPUT;
  avr_load_firmware(avr, &f);

  // Gnarly poking here replaces the default _avr_io_console_write
  // callback for the simavr console with our implementation that
  // allows escape sequences to be printed
  avr->io[AVR_DATA_TO_IO(f.console_register_addr)].w.c = console_write;

  int state;
  for (;;) {
    state = avr_run(avr);
    if (state == cpu_Done || state == cpu_Crashed)
      break;
  }

  avr_terminate(avr);
  return state == cpu_Crashed;
}
