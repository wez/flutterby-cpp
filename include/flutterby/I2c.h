#pragma once
#include "flutterby/Types.h"
#include "flutterby/Result.h"

namespace flutterby {

namespace I2cMaster {
enum class Error {
  BusFault, // A TWI bus fault occurred while attempting to capture the bus
  BusCaptureTimeout, // timed out waiting for the bus to be ready
  SlaveResponseTimeout, // No ACK from slave within the timeout period
  SlaveNotReady, // Slave NACKd the START condition
  SlaveNack, // Slave NACKd during data transfer
};
using I2cResult = Result<Unit, Error>;

void enable(uint32_t bus_frequency);
void disable();

// Synchronously read data into destBuf
I2cResult read_buffer(
    uint8_t slave_address,
    uint16_t timeout_ms,
    uint8_t read_address,
    uint8_t* destBuf,
    uint16_t destLen);

// Synchronously read data into dest
template <typename T>
I2cResult read(
    uint8_t slave_address,
    uint16_t timeout_ms,
    uint8_t read_address,
    T& dest) {
  return read_buffer(
      slave_address,
      timeout_ms,
      read_address,
      reinterpret_cast<uint8_t*>(&dest),
      sizeof(T));
}

// Synchronously write data from src_buf
I2cResult write_buffer(
    uint8_t slave_address,
    uint16_t timeout_ms,
    uint8_t write_address,
    const uint8_t* src_buf,
    uint16_t srcLen);

// Synchronously write data from src
template <typename T>
I2cResult write(
    uint8_t slave_address,
    uint16_t timeout_ms,
    uint8_t write_address,
    const T& src) {
  return write_buffer(
      slave_address,
      timeout_ms,
      write_address,
      reinterpret_cast<const uint8_t*>(&src),
      sizeof(T));
}
}
}
