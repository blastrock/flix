#include "Interrupt.hpp"
#include "io.hpp"
#include "Debug.hpp"

extern "C" void intHandler(InterruptState* s)
{
  InterruptHandler::handle(s);
}

void InterruptHandler::handle(InterruptState* s)
{
  if (s->intNo <= 47)
  {
    if (s->intNo >= 40)
      io::outb(0xA0, 0x20);
    io::outb(0x20, 0x20);
  }

  if (s->intNo < 32)
  {
    fDeg() << "Isr " << s->intNo << '!';
    PANIC("exception");
  }
  else
  {
    fDeg() << "Isr " << (s->intNo - 32) << '!';
  }
}
