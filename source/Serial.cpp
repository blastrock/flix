#include "Serial.hpp"
#include <new>
#include <sstream>
#include "io.hpp"
#include "Screen.hpp"

// http://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming

enum Registers
{
  REG_DATA = 0,
  REG_INT_ENABLE,
  REG_DIVLOW = 0,
  REG_DIVHIGH,
  REG_FIFO_CONTROL,
  REG_LINE_CONTROL,
  REG_MODEM_CONTROL,
  REG_LINE_STATUS,
  REG_MODEM_STATUS,
  REG_SCRATCH
};

void Serial::init()
{
  // disable interrupts
  io::outb(_port + REG_INT_ENABLE, 0x00);

  // set DLAB
  io::outb(_port + REG_LINE_CONTROL, 0x80);

  // set baud rate (max)
  io::outb(_port + REG_DIVLOW, 12);
  io::outb(_port + REG_DIVHIGH, 0);

  // unset DLAB, 8 bit, no parity, one stop bit
  io::outb(_port + REG_LINE_CONTROL, 0x03);

  // enable fifo, clear them, 14 bytes
  io::outb(_port + REG_FIFO_CONTROL, 0xC7);

  // IRQs enabled, RTS/DSR set
  io::outb(_port + REG_MODEM_CONTROL, 0x0B);
}

void Serial::write(char c)
{
  int i = 0;
  while ((io::inb(_port + REG_LINE_STATUS) & 0x20) == 0)
    ++i;
  io::outb(_port + REG_DATA, c);
}

Serial* getCom1()
{
  static char buf[sizeof(Serial)];
  static Serial* com1 = 0;
  if (!com1)
  {
    com1 = new(buf) Serial(0x3f8);
    com1->init();
  }
  return com1;
}
