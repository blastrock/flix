#include "Debug.hpp"
#include "PageDirectory.hpp"

void printStackTrace(uint64_t stackPointer)
{
  // stackFrame[0] is the return stack pointer and stackFrame[1] is the return
  // instraction pointer
  void** stackFrame = reinterpret_cast<void**>(stackPointer);

  Degf("Stack trace:");
  while (stackFrame[1])
  {
    Degf("%p", stackFrame[1]);
    if (stackFrame >= stackFrame[0])
    {
      Degf("stack fail");
      return;
    }

    stackFrame = static_cast<void**>(stackFrame[0]);

    if (!PageDirectory::getCurrent()->isPageMapped(stackFrame))
    {
      Degf("Invalid stack pointer");
      return;
    }
  }
}

// make a non-inline function
void PANIC2(const char* msg)
{
  asm volatile ("cli");
  asm volatile ("xchgw %bx, %bx");
  while (true)
    asm volatile ("hlt");
  //PANIC(msg);
}
