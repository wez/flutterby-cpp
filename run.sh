#!/bin/bash
example=$1
PORT=/dev/cu.usbmodem1431
set -e

MCU=atmega32u4
elf=target/$MCU/$example.elf

make $elf

avr-objcopy $elf -O ihex target/$MCU/$example.hex

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

avrdude -p atmega32u4 -U flash:w:target/$MCU/$example.hex:i -cavr109 -b57600 -D -P $PORT
