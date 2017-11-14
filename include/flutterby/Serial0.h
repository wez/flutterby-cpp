#pragma once
#include "avr_autogen.h"
#include "flutterby/Debug.h"
#include "flutterby/Types.h"

namespace flutterby {

class Serial0 {
 public:
  static void configure(u32 baud) {
    // Use 2x speed mode
    Usart0::ucsr0a |= Usart0Ucsr0aFlags::U2X0;
    u16 setting = (F_CPU / 4 / baud - 1) / 2;
    if (((F_CPU == 16000000UL) && (baud == 57600)) || (setting > 4095)) {
      setting = (F_CPU / 8 / baud - 1) / 2;
      Usart0::ucsr0a.clear();
    }
    Usart0::ubrr0 = setting;

    // Enable Rx and Tx.
    Usart0::ucsr0b |= Usart0Ucsr0bFlags::RXEN0 | Usart0Ucsr0bFlags::TXEN0;

    // Set the data bit register to 8n1.
    // Note that UCSZ0 is really a mask but happens to evaluate to the
    // correct value for 8bit character size.
    Usart0::ucsr0c = // Usart0Ucsr0cFlags::UMSEL0_SYNCHRONOUS_USART |
        Usart0Ucsr0cFlags::UPM0_DISABLED /* no parity */ |
        Usart0Ucsr0cFlags::USBS0_1BIT /* 1 stop bit */ |
        Usart0Ucsr0cFlags::UCSZ0 /* 8bit (really a mask)*/;
  }

  static inline void write_byte_immediate(u8 b) {
    // Wait for empty transmit buffer
    while ((Usart0::ucsr0a & Usart0Ucsr0aFlags::UDRE0) !=
           Usart0Ucsr0aFlags::UDRE0) {
      ;
    }
    Usart0::udr0 = b;
    Usart0::ucsr0a |=
        Usart0Ucsr0aFlags::TXC0; // clear this bit by writing a 1 to it
  }
};

class Serial0TxStream {
  public:
    void operator()(u8 b) {
      Serial0::write_byte_immediate(b);
    }
};

#define TXSER() FormatStream<Serial0TxStream, kFormatStreamCRLF>().stream()


}
