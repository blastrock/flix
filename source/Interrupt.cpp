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
    fDeg() << "Isr " << (int)s->intNo << '!';
PANIC("no");
    switch (s->intNo)
    {
      case 14:
        fDeg() << "Page fault";
        fDeg() << "Page was " << (s->errCode & 1 ? "present" : "not present");
        fDeg() << "Fault on " << (s->errCode & 2 ? "write" : "read");
        if (s->errCode & 4)
          fDeg() << "Invalid reserved field!";
        fDeg() << "Fault on " << (s->errCode & 8 ? "fetch" : "execute");
        {
          uint64_t address;
          asm volatile(
              "movq %%cr2, %0"
              :"=r"(address)
              );
          fDeg() << "Fault on " << std::hex << address;
        }
    }

    fDeg() << "stack: " << &s->rip;
    fDeg() << "RIP: " << std::hex << s->rip;
    fDeg() << "RSP: " << std::hex << s->rsp;
    PANIC("exception");
  }
  else
  {
    fDeg() << "Isr " << (int)(s->intNo - 32) << '!';
  }
}
