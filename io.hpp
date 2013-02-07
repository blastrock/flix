#ifndef IO_H
#define IO_H

namespace io
{

inline void outb(u16 port, u8 value)
{
  asm volatile (
      "outb %1, %0"
      :
      : "dN"(port), "a"(value));
}

inline u8 inb(u16 port)
{
  u8 value;
  asm volatile (
      "inb %1, %0"
      : "=a"(value)
      : "dN"(port));
  return value;
}

}

#endif /* IO_H */
