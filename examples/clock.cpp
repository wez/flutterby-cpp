#include "avr_autogen.h"
#include "flutterby/EventLoop.h"
#include "flutterby/Debug.h"
#include "flutterby/Serial0.h"
#include "flutterby/I2c.h"
#include "flutterby/Gpio.h"

/** This is intended to drive an atmega328p in the Solder Time Desk Clock */

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

  auto timer = make_timer(2_s, true, [] {
                 TXSER() << "Boop"_P;
                 auto res = read_time();
                 if (res.is_ok()) {
                   auto time = move(res.value());
                   TXSER() << time.hours << ":"_P << time.minutes << ":"_P
                           << time.seconds << " day:"_P << time.day
                           << " date:"_P << time.date << " month:"_P
                           << time.month << " year:"_P << time.year;
                 } else {
                   TXSER() << "failed to read i2c: "_P << u8(res.error());
                 }
               }).value();
  eventloop::enable_timer(timer);

  int row_num = 0;
  int col_num = 0;

  auto led_timer = make_timer(1_s, true, [&row_num, &col_num]() mutable {
                     auto decoder = col_num / 8;
                     auto sub_col = col_num % 8;
                     TXSER() << "row: "_P << row_num << " col: "_P << col_num
                             << " decoder:"_P << decoder << " sub_col:"_P
                             << sub_col;

                     // Deactivate LEDs before we select a different
                     // column to avoid visual artifacts
                     RowPins::write(0);
                     select_decoder(decoder);
                     select_column(col_num % 8);
                     RowPins::write(1<<row_num);

                     if (++col_num > 19) {
                       col_num = 0;

                       if (++row_num > 6) {
                         row_num = 0;
                       }
                     }

                   }).value();
  eventloop::enable_timer(led_timer);

  eventloop::run_forever();

  return 0;
}
