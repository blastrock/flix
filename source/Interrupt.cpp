#include "Interrupt.hpp"
#include "io.hpp"
#include "Debug.hpp"
#include "Task.hpp"
#include "Syscall.hpp"

extern "C" void intHandler(InterruptState* s)
{
  InterruptHandler::handle(s);
}

struct ScopedExceptionHandling
{
  static bool busy;

  ScopedExceptionHandling()
  {
    if (busy)
      PANIC("Nested exception");
    busy = true;
  }
  ~ScopedExceptionHandling()
  {
    busy = false;
  }
};

bool ScopedExceptionHandling::busy = false;

void InterruptHandler::handle(InterruptState* s)
{
  // acknowledge interrupt
  if (s->intNo <= 47)
  {
    if (s->intNo >= 40)
      io::outb(0xA0, 0x20);
    io::outb(0x20, 0x20);
  }

  if (s->intNo < 32)
  {
    Degf("Isr %d!", s->intNo);

    switch (s->intNo)
    {
      case 13:
        Degf("General-Protection Exception (%d)", s->errCode);
        break;
      case 14:
        Degf("Page fault");
        Degf("Page was %s", s->errCode & 1 ? "present" : "not present");
        Degf("Fault on %s", s->errCode & 2 ? "write" : "read");
        Degf("Access was %s",
            s->errCode & 4 ? "unpriviledged" : "priviledged");
        Degf("Fault on %s", s->errCode & 8 ? "fetch" : "execute");
        {
          uint64_t address;
          asm volatile(
              "movq %%cr2, %0"
              :"=r"(address)
              );
          Degf("Fault on %x", address);
        }
        break;
    }

    Degf("RIP: %x", s->rip);
    Degf("RSP: %x", s->rsp);

    {
      ScopedExceptionHandling scope;

      uint64_t rbp;
      asm volatile ("mov %%rbp, %0":"=r"(rbp)::);

      printStackTrace(rbp);

      PageDirectory::getKernelDirectory()->use();

      TaskManager::get()->terminateCurrentTask();
    }

    TaskManager::get()->scheduleNext();
  }
  else if (s->intNo < 48)
  {
    uint8_t intNo = s->intNo - 32;
    Degf("Int %x!", intNo);
    if (intNo == 0)
    {
      Task::Context context;
      context.r15 = s->r15;
      context.r14 = s->r14;
      context.r13 = s->r13;
      context.r12 = s->r12;
      context.rbx = s->rbx;
      context.rbp = s->rbp;
      context.r11 = s->r11;
      context.r10 = s->r10;
      context.r9 = s->r9;
      context.r8 = s->r8;
      context.rax = s->rax;
      context.rcx = s->rcx;
      context.rdx = s->rdx;
      context.rsi = s->rsi;
      context.rdi = s->rdi;
      context.rip = s->rip;
      context.cs = s->cs;
      context.rflags = s->rflags;
      context.rsp = s->rsp;
      context.ss = s->ss;
      TaskManager::get()->saveCurrentTask(context);
      TaskManager::get()->scheduleNext();
    }
  }
  else
  {
    Degf("SYSCALL %d", s->intNo, s->rax);

    switch (s->rax)
    {
    case sys::exit:
      PageDirectory::getKernelDirectory()->use();
      TaskManager::get()->terminateCurrentTask();
      TaskManager::get()->scheduleNext();
      break;
    default:
      Degf("Unknown syscall");
      break;
    }
  }
}
