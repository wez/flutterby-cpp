#include "avr_autogen.h"
#include "flutterby/EventLoop.h"
#include "flutterby/Debug.h"
#include "flutterby/Serial0.h"
#include "flutterby/I2c.h"
#include "flutterby/Gpio.h"
#include "flutterby/Timer0.h"

#include "gfxfont.h"
#include "TomThumb.h"

/** This is intended to drive an atmega328p in the Solder Time Desk Clock.
 * Schematic for the board is here:
 * http://www.spikenzielabs.com/Downloadables/stdeskclock/STDC_Schematic.pdf
 */

using namespace flutterby;

enum Ds1337Consts {
  TWI_ADDR = 0x68,
  SECONDS = 0x00,
  MINUTES = 0x01,
  HOURS = 0x02,
  DAY = 0x03,
  DATE = 0x04,
  MONTH = 0x05,
  YEAR = 0x06,
};

struct bcd_time {
  u8 seconds;
  u8 minutes;
  u8 hours;
  u8 day; // day of the week. 0 == Sunday
  u8 date;
  u8 month;
  u8 year;
};

static bcd_time last_read_time;

static inline u8 decode_bcd(u8 x) {
  return ((x >> 4) * 10 + (x & 0x0F));
}

Result<bcd_time, I2cMaster::Error> read_time() {
  bcd_time time;
  auto res = I2cMaster::read(TWI_ADDR, 100, SECONDS, time);
  if (res.is_err()) {
    return Error<bcd_time>(move(res.error()));
  }

  time.seconds = decode_bcd(time.seconds);
  time.minutes = decode_bcd(time.minutes);
  time.hours = decode_bcd(time.hours);
  time.day = decode_bcd(time.day);
  time.date = decode_bcd(time.date);
  time.month = decode_bcd(time.month);
  time.year = decode_bcd(time.year);

  return Ok<bcd_time, I2cMaster::Error>(move(time));
}

// Portb is attached to the 7 LED display rows.
// Set them to output mode.
using Row0 = gpio::OutputPin<gpio::PortB, 0>;
using Row1 = gpio::OutputPin<gpio::PortB, 1>;
using Row2 = gpio::OutputPin<gpio::PortB, 2>;
using Row3 = gpio::OutputPin<gpio::PortB, 3>;
using Row4 = gpio::OutputPin<gpio::PortB, 4>;
using Row5 = gpio::OutputPin<gpio::PortB, 5>;
using Row6 = gpio::OutputPin<gpio::PortB, 6>;
using RowPins = gpio::OutputPins<Row0, Row1, Row2, Row3, Row4, Row5, Row6>;

// S1 and S2 select one of the sn74ls138n chips.  They are connected
// to the G1,G2A,G2B pins in different combinations such that the
// levels in the table below activate a different instance of the decoder:
// S1 | S2 | decoder
// H  | L  | 1
// L  | H  | 2
// L  | L  | 3
// H  | H  | None
using S1 = gpio::OutputPin<gpio::PortC, 2>;
using S2 = gpio::OutputPin<gpio::PortC, 3>;
using DecoderSelect = gpio::OutputPins<S1, S2>;

// SA,SB,SC are connected to the sn74ls138n chips pins A-C.  The S1,S2
// combination selects the instance of the decoder, and the SA-SC bits
// reference a column in the associated LED matrix; there are thus
// 3x8 = 24 potential columns that could be addressed, but the physical
// LED matrices provide a total of 20 columns
using SA = gpio::OutputPin<gpio::PortD, 4>;
using SB = gpio::OutputPin<gpio::PortD, 5>;
using SC = gpio::OutputPin<gpio::PortD, 6>;
using ColumnSelect = gpio::OutputPins<SA, SB, SC>;

// Button 0 -> INT0
using Button0 = gpio::InputPin<gpio::PortD, 2, gpio::kEnablePullUp>;
// Button 1 -> INT1
using Button1 = gpio::InputPin<gpio::PortD, 3, gpio::kEnablePullUp>;

static void select_decoder(u8 n) {
  switch (n) {
    case 0:
      DecoderSelect::write(0b01);
      break;
    case 1:
      DecoderSelect::write(0b10);
      break;
    case 2:
      DecoderSelect::write(0b00);
      break;
    default:
      DecoderSelect::write(0b11);
  }
}

static void select_column(u8 col) {
  ColumnSelect::write(col);
}

// We use double buffering so that writes to the screen
// don't flicker
static volatile u8 matrix_data[2][20];
static volatile u8 active_matrix = 0;
static volatile u8 brightness = 0x20;

// Button 0 pressed
IRQ_INT0 {
  brightness += 0x10;
}

static void clear_screen() {
  u8 screen = (active_matrix + 1) & 1;
  for (u8 i = 0; i < sizeof(matrix_data[screen]); ++i) {
    matrix_data[screen][i] = 0;
  }
}
static void next_screen() {
  active_matrix = (active_matrix + 1) & 1;
}

u8 render_char_at(u8 screen, u8 x, u8 y, u8 c) {
  c -= progmem_deref(&TomThumb.first);
  auto glyph = &(progmem_deref(&TomThumb.glyph)[c]);
  auto bitmap = progmem_deref(&TomThumb.bitmap);

  auto bo = progmem_deref(&glyph->bitmapOffset);
  auto w = progmem_deref(&glyph->width);
  auto h = progmem_deref(&glyph->height);
  int8_t xo = progmem_deref(&glyph->xOffset),
         yo = progmem_deref(&glyph->yOffset);
  uint8_t xx, yy, bits = 0, bit = 0;

  for (yy = 0; yy < h; yy++) {
    for (xx = 0; xx < w; xx++) {
      if (!(bit++ & 7)) {
        bits = progmem_deref(&bitmap[bo++]);
      }
      auto target_x =  x + xo + xx;
      auto target_y = y + yo + yy;
      if ((bits & 0x80) && target_x < 20 && target_x >= 0) {
        matrix_data[screen][target_x] |= 1 << target_y;
      }
      bits <<= 1;
    }
  }

  // Return the visible width of the character
  return progmem_deref(&glyph->xAdvance);
}

class ThumbStream {
  u8 x_{0};
 public:

  void operator()(uint8_t b) {
    if (x_ >= 20) {
      return;
    }
    u8 screen = (active_matrix + 1) & 1;
    x_ += render_char_at(screen, x_, 7, b);
  }
};

#define MATRIX() FormatStream<ThumbStream, kFormatStreamNone>().stream()

static u8 long_message[] = "hello there, scroll me";
static u8 long_message_len = 22;
static u8 long_message_pos = 0;

static void led_tick() {
  static volatile int col_num = 0;

  auto decoder = col_num / 8;
  auto sub_col = col_num % 8;

  select_decoder(decoder);
  select_column(col_num % 8);
  RowPins::write(matrix_data[active_matrix][col_num]);

  // PWM to select brightness level
  for (u8 i = 0; i < brightness; ++i) {
    __asm__ __volatile__ ("nop");
    __asm__ __volatile__ ("nop");
  }
  RowPins::write(0);
  for (u8 i = brightness; i!= 0; ++i) {
    __asm__ __volatile__ ("nop");
    __asm__ __volatile__ ("nop");
  }

  if (++col_num > 19) {
    col_num = 0;
  }
}

IRQ_TIMER0_COMPA {
  led_tick();
}

int main() {
  __builtin_avr_cli();
#ifdef HAVE_AVR_WDT
  __builtin_avr_wdr();
  Wdt::wdtcsr = WdtWdtcsrFlags::WDCE | WdtWdtcsrFlags::WDE;
  Wdt::wdtcsr.clear();
  Cpu::mcusr ^= CpuMcusrFlags::WDRF;
#endif
  I2cMaster::enable(400000);
  Serial0::configure(9600);

  RowPins::setup();
  DecoderSelect::setup();
  ColumnSelect::setup();

  // Turn on interrupts for the buttons so that we can adjust
  // the brightness of the display
  Exint::eicra |= ExintEicraFlags::ISC0_RISING_EDGE_OF_INTX;
  // This is really a mask but since it covers both int0 and int1,
  // we're good to just use that here
  Exint::eimsk.raw_bits() = 1 << 0;

  Timer0::configure(
      Timer0::WaveformGenerationMode::ClearOnTimerMatchOutputCompare,
      100);

  clear_screen();
  MATRIX() << "w00t!!!"_P;
  next_screen();

  // This periodic task is responsible for re-reading from the RTC.
  // We do this approximately every second and store the value in
  // the global last_read_time variable.
  eventloop::enable_timer(make_timer(1_s, true, [] {
                            auto res = read_time();
                            if (res.is_ok()) {
                              last_read_time = move(res.value());
#if 0
                   TXSER() << time.hours << ":"_P << time.minutes << ":"_P
                           << time.seconds << " day:"_P << time.day
                           << " date:"_P << time.date << " month:"_P
                           << time.month << " year:"_P << time.year;
#endif
                            }
                          }).value());

#if 1
  eventloop::enable_timer(make_timer(2_s, true, [] {
                            auto out = MATRIX();
                            clear_screen();
                            if (last_read_time.hours < 10) {
                              out << "0"_P;
                            }
                            out << last_read_time.hours;
                            out << ":"_P;
                            if (last_read_time.minutes < 10) {
                              out << "0"_P;
                            }
                            out << last_read_time.minutes;
                            next_screen();
                          }).value());
#else
  eventloop::enable_timer(make_timer(200_ms, true, [] {
                            clear_screen();
                            u8 x = 0;
                            u8 i = long_message_pos;
                            u8 screen = (active_matrix + 1) & 1;

                            while (x < 20) {
                              x +=
                                  render_char_at(screen, x, 7, long_message[i]);
                              i = (i + 1) % long_message_len;
                            }

                            long_message_pos =
                                (long_message_pos + 1) % long_message_len;
                            next_screen();
                          }).value());
#endif

  eventloop::run_forever();

  return 0;
}
