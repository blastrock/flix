#include "Debug.hpp"
#include "PageDirectory.hpp"

XLL_LOG_CATEGORY("support/debug");

void printStackTrace()
{
  uint64_t rbp;
  asm volatile ("mov %%rbp, %0":"=r"(rbp));
  printStackTrace(rbp);
}

void printStackTrace(uint64_t stackPointer)
{
  // stackFrame[0] is the return stack pointer and stackFrame[1] is the return
  // instraction pointer
  void** stackFrame = reinterpret_cast<void**>(stackPointer);

  xDeb("Stack trace:");
  while (true)
  {
    if (!PageDirectory::getCurrent()->isPageMapped(stackFrame) ||
        !PageDirectory::getCurrent()->isPageMapped(stackFrame+1))
    {
      xDeb("Invalid pointer: %p", stackFrame);
      return;
    }

    if (!stackFrame[1])
      break;

    xDeb("%p", stackFrame[1]);
    if (stackFrame >= stackFrame[0])
    {
      xDeb("Stack going backward, aborting");
      return;
    }

    stackFrame = static_cast<void**>(stackFrame[0]);
  }
}
