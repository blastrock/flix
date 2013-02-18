#include "Isr.hpp"
#include "Screen.hpp"

struct Registers
{
  uint32_t ds;                  // Data segment selector
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
  uint32_t int_no, err_code;    // Interrupt number and error code (if applicable)
  uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
};

extern "C" void isr_handler(Registers regs)
{
  if (regs.int_no <= 47)
  {
    if (regs.int_no >= 40)
      io::outb(0xA0, 0x20);
    io::outb(0x20, 0x20);
  }

  if (regs.int_no < 32)
  {
    Screen::putString("Isr ");
    Screen::putInt(regs.int_no);
    Screen::putString("!\n");
  }
  else
  {
    Screen::putString("Irq ");
    Screen::putInt(regs.int_no - 32);
    Screen::putString("!\n");
  }
}
