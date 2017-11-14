#include "flutterby/I2c.h"
#include "flutterby/BusyWait.h"
#include "avr_autogen.h"

static constexpr uint8_t TWI_ADDRESS_READ = 0x01;
static constexpr uint8_t TWI_ADDRESS_WRITE = 0x00;
static constexpr uint8_t TWI_DEVICE_ADDRESS_MASK = 0xFE;

namespace flutterby {
namespace I2cMaster {

enum TwiStatus {
  BusError = 0x00,
  Start = 0x8,
  RepeatStart = 0x10,
  XmitAckSLA = 0x18,
  XmitNackSLA = 0x20,
  XmitAckData = 0x28,
  XmitNackData = 0x30,
  MasterArbitrationLost = 0x38,
  RxAckSLA = 0x40,
  RxNackSLA = 0x48,
  RxAckData = 0x50,
  RxNackData = 0x58,
  NoInfo = 0xf8,
};

static inline TwiStatus get_status() {
  return TwiStatus(Twi::twsr.raw_bits() & 0b11111000);
}

void enable(uint32_t bus_frequency) {
  // Set to input and internal pull-ups on SDA, SCL
  Portd::ddrd &= ~(PortdSignalFlags::PD0 | PortdSignalFlags::PD1);
  Portd::portd |= PortdSignalFlags::PD0 | PortdSignalFlags::PD1;

  Twi::twsr = TwiTwsrFlags::TWPS_1; // no prescaling
  Twi::twbr = (((F_CPU / bus_frequency) - 16) / 2);
}

void disable() {
  Twi::twcr &= ~TwiTwcrFlags::TWEN;
}

inline void set_twcr(bitflags<TwiTwcrFlags::TwiTwcrFlags, u8> value) {
  Twi::twcr = value;
#ifdef HAVE_SIMAVR
  // Force a short wait before we try to read back from the register
  // as the simulator seems to have some timing issues.  Also worth
  // noting that the busy wait is wildly inaccurate under simavr anyway(!)
  busy_wait_us(10);
#endif
}

// Helper to make sure that we release the bus.
// The bus starts out unowned, but is then claimed by calling the start()
// method.  Reads will typically write the register number and then read
// the data back from the device, which results in two start() calls.
// The bus is considered to be owned from the first start() call onwards.
// The bus is released automatically when this xmit helper falls out of scope.
class TwiXmit {
  bool owned_;

 public:
  TwiXmit() : owned_(false) {}
  TwiXmit(const TwiXmit&) = delete;

  ~TwiXmit() {
    stop();
  }

  I2cResult start(const uint8_t slave_address, const uint8_t timeout_ms) {
    uint16_t timeout_remaining;

    timeout_remaining = (timeout_ms * 100);
    set_twcr(TwiTwcrFlags::TWINT | TwiTwcrFlags::TWSTA | TwiTwcrFlags::TWEN);

    while (timeout_remaining) {
      if (Twi::twcr & TwiTwcrFlags::TWINT) {
        switch (get_status()) {
          case TwiStatus::Start:
          case TwiStatus::RepeatStart:
            goto captured;
          case TwiStatus::MasterArbitrationLost:
            set_twcr(
                TwiTwcrFlags::TWINT | TwiTwcrFlags::TWSTA | TwiTwcrFlags::TWEN);
            // Restart bus arbitration with the full timeout
            timeout_remaining = (timeout_ms * 100);
            continue;
          default:
            Twi::twcr = TwiTwcrFlags::TWEN;
            return I2cResult::Error(Error::BusFault);
        }
      }
      busy_wait_us(10);
      timeout_remaining--;
    }

    set_twcr(TwiTwcrFlags::TWEN);
    return I2cResult::Error(Error::BusCaptureTimeout);

  captured:

    Twi::twdr = slave_address;
    set_twcr(TwiTwcrFlags::TWINT | TwiTwcrFlags::TWEN);

    timeout_remaining = (timeout_ms * 100);
    while (timeout_remaining) {
      if (Twi::twcr & TwiTwcrFlags::TWINT) {
        goto check_status;
      }
      timeout_remaining--;
      busy_wait_us(10);
    }

    return I2cResult::Error(Error::SlaveResponseTimeout);

  check_status:
    auto s = get_status();
    switch (s) {
      case XmitAckSLA:
      case RxAckSLA:
#ifdef HAVE_SIMAVR
      // AFAICT, this is an invalid result here, but I see this
      // coming back from simavr
      case XmitAckData:
#endif
        owned_ = true;
        return I2cResult::Ok();
      default:
        DBG() << "bad state: "_P << u8(s);
        set_twcr(
            TwiTwcrFlags::TWINT | TwiTwcrFlags::TWSTO | TwiTwcrFlags::TWEN);
        return I2cResult::Error(Error::SlaveNotReady);
    }
  }

  // Sends a TWI STOP onto the TWI bus, terminating communication with the
  // currently addressed device.
  void stop() {
    if (!owned_) {
      return;
    }
    set_twcr(TwiTwcrFlags::TWINT | TwiTwcrFlags::TWSTO | TwiTwcrFlags::TWEN);
    // Wait for bus to release (this is important for the ergodox!)
    while (Twi::twcr & TwiTwcrFlags::TWSTO)
      ;
    owned_ = false;
  }

  I2cResult recv_byte(uint8_t* dest, const bool isFinalbyte) {
    if (isFinalbyte) {
      set_twcr(TwiTwcrFlags::TWINT | TwiTwcrFlags::TWEN);
    } else {
      set_twcr(TwiTwcrFlags::TWINT | TwiTwcrFlags::TWEN | TwiTwcrFlags::TWEA);
    }

    while (!(Twi::twcr & TwiTwcrFlags::TWINT))
      ;
    *dest = Twi::twdr;

    auto status = get_status();

    switch (status) {
      case RxAckData:
        return I2cResult::Ok();
      case RxNackData:
        if (isFinalbyte) {
          return I2cResult::Ok();
        }
    }

    DBG() << "recv_byte: status is "_P << u8(status);
    return I2cResult::Error(Error::SlaveNack);
  }

  I2cResult send_byte(const uint8_t byte) {
    Twi::twdr = byte;
    Twi::twcr = TwiTwcrFlags::TWINT | TwiTwcrFlags::TWEN;
    while (!(Twi::twcr & TwiTwcrFlags::TWINT))
      ;

    switch (get_status()) {
      case XmitAckData:
        return I2cResult::Ok();
      default:
        return I2cResult::Error(Error::SlaveNack);
    }
  }
};

I2cResult read_buffer(
    uint8_t slave_address,
    uint16_t timeout_ms,
    uint8_t read_address,
    uint8_t* destBuf,
    uint16_t destLen) {
  slave_address <<= 1;
  TwiXmit xmit;
  Try(xmit.start(
      (slave_address & TWI_DEVICE_ADDRESS_MASK) | TWI_ADDRESS_WRITE,
      timeout_ms));

  Try(xmit.send_byte(read_address));

  Try(xmit.start(
      (slave_address & TWI_DEVICE_ADDRESS_MASK) | TWI_ADDRESS_READ,
      timeout_ms));

  while (destLen--) {
    Try(xmit.recv_byte(destBuf++, destLen == 0));
  }

  return I2cResult::Ok();
}

Result<Unit, Error> write_buffer(
    uint8_t slave_address,
    uint16_t timeout_ms,
    uint8_t write_address,
    const uint8_t* src_buf,
    uint16_t srcLen) {
  slave_address <<= 1;
  TwiXmit xmit;

  Try(xmit.start(
      (slave_address & TWI_DEVICE_ADDRESS_MASK) | TWI_ADDRESS_WRITE,
      timeout_ms));

  Try(xmit.send_byte(write_address));

  while (srcLen--) {
    Try(xmit.send_byte(*src_buf++));
  }

  return I2cResult::Ok();
}
}
}
