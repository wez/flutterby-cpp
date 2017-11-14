#include "avr_autogen.h"
#include "flutterby/Test.h"
#include "flutterby/I2c.h"
#include "flutterby/BusyWait.h"

// This test is intended to exercise the I2c code rather than
// be a formal driver for a ds1338.  I do want to add a driver
// for a ds1337, which is similar.

using namespace flutterby;

enum Ds1338Consts {
  TWI_ADDR = 0x68,
  SECONDS = 0x00,
  MINUTES = 0x01,
  HOURS = 0x02,
  DAY = 0x03,
  DATE = 0x04,
  MONTH = 0x05,
  YEAR = 0x06,
  CONTROL = 0x07,
  /*
   * Seconds register flag - oscillator is enabled when
   * this is set to zero. Undefined on startup.
   */
  CH = 7,
};

struct bcd_time {
  u8 seconds;
  u8 minutes;
  u8 hours;
  u8 day;
  u8 date;
  u8 month;
  u8 year;
};

static inline u8 decode_bcd(u8 x) {
  return ((x >> 4) * 10 + (x & 0x0F));
}

I2cMaster::I2cResult init() {
  u8 seconds;
  Try(I2cMaster::read(TWI_ADDR, 1000, SECONDS, seconds));
  seconds &= ~(1<<CH);
  Try(I2cMaster::write(TWI_ADDR, 1000, SECONDS, seconds));

  return I2cMaster::I2cResult::Ok();
}

I2cMaster::I2cResult test() {
  Try(init());

  while (true) {
    bcd_time time;
    Try(I2cMaster::read(TWI_ADDR, 1000, SECONDS, time));

    time.seconds = decode_bcd(time.seconds);
    time.minutes = decode_bcd(time.minutes);
    time.hours = decode_bcd(time.hours);
    time.day = decode_bcd(time.day);
    time.date = decode_bcd(time.date);
    time.month = decode_bcd(time.month);
    time.year = decode_bcd(time.year);

#if 0
    DBG() << "seconds value is "_P << time.seconds;
    DBG() << "minutes value is "_P << time.minutes;
    DBG() << "hours value is "_P << time.hours;
    DBG() << "day value is "_P << time.day;
    DBG() << "date value is "_P << time.date;
    DBG() << "month value is "_P << time.month;
    DBG() << "year value is "_P << time.year;
#endif

    if (time.seconds >= 2) {
      break;
    }
    // We'd like to wait for 1 second here, but the busy waits
    // don't try to respect the clock speed of the host in simavr
    // (https://github.com/buserror/simavr/issues/201) so this line
    // doesn't really do very much!
    busy_wait_ms(1000);
  }

  return I2cMaster::I2cResult::Ok();
}

int main() {
  I2cMaster::enable(400000);

  auto res = test();
  if (res.is_err()) {
    DBG() << "result error: "_P << u8(res.error());
  }
  EXPECT(test().is_ok());

  return 0;
}
