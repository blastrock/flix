#include "Isr.hpp"
#include "Screen.hpp"

struct Registers
{
  u32 ds;                  // Data segment selector
  u32 edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
  u32 int_no, err_code;    // Interrupt number and error code (if applicable)
  u32 eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
};

extern "C" void isr_handler(Registers regs)
{
  Screen::putString("Int ");
  Screen::putInt(regs.int_no);
  Screen::putString("!\n");
}
