#include "avr_autogen.h"
#include "flutterby/EventLoop.h"
#include "flutterby/Debug.h"
#include "flutterby/Serial0.h"
#include "flutterby/I2c.h"

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
  eventloop::run_forever();

  return 0;
}
