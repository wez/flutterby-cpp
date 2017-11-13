#!/bin/bash
example=$1
PORT=/dev/cu.usbserial-A106TF8G
set -e

MCU=atmega328p
F_CPU=16000000
elf=target/$MCU/examples/$example.elf
hex=target/$MCU/examples/$example.hex

make MCU=$MCU F_CPU=$F_CPU $elf

avr-objcopy $elf -O ihex $hex

if [[ ! -e $PORT ]] ; then
  echo "$PORT is not present."
  echo "Press the reset button on the board?"
  while [[ ! -e $PORT ]] ; do
    sleep 1
  done
fi

if [[ -e $PORT ]]; then
  case $(uname) in
    Darwin)
      stty -f $PORT ispeed 1200 ospeed 1200
      ;;
    Linux)
      stty -F $PORT ispeed 1200 ospeed 1200
      ;;
  esac
fi

avrdude -p $MCU -U flash:w:$hex:i -carduino -b57600 -D -P $PORT
