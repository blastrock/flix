#include "Interrupt.hpp"
#include "io.hpp"
#include "Debug.hpp"
#include "TaskManager.hpp"
#include "Syscall.hpp"
#include "Keyboard.hpp"
#include "DescTables.hpp"

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
  assert((s->cs == DescTables::SYSTEM_CS ||
        (s->rflags & (1 << 9))) &&
      "Interrupts were disabled in a user task");

  // acknowledge interrupt
  if (s->intNo <= 47)
  {
    if (s->intNo >= 40)
      io::outb(0xA0, 0x20);
    io::outb(0x20, 0x20);
  }

  if (s->intNo < 32) // CPU exception
  {
    Degf("Isr %d!", s->intNo);

    switch (s->intNo)
    {
      case 13:
        Degf("General-Protection Exception (%d)", s->errCode);
        break;
      case 14:
      {
        void* address;
        asm volatile(
            "movq %%cr2, %0"
            :"=r"(address)
            );

        Degf("Page fault on %p", address);

        if (PageDirectory::getCurrent()->handleFault(address))
          return;

        Degf("Page was %s", s->errCode & 1 ? "present" : "not present");
        Degf("Fault on %s", s->errCode & 2 ? "write" : "read");
        Degf("Access was %s",
            s->errCode & 4 ? "unprivileged" : "privileged");
        Degf("Fault on %s", s->errCode & 8 ? "fetch" : "execute");
      }
        break;
    }

    Degf("RIP: %x", s->rip);
    Degf("RSP: %x", s->rsp);

    {
      ScopedExceptionHandling scope;

      printStackTrace(s->rbp);

      PageDirectory::getKernelDirectory()->use();

      TaskManager::get()->terminateCurrentTask();
    }

    TaskManager::get()->scheduleNext();
  }
  else if (s->intNo < 48) // PIC interrupt
  {
    uint8_t intNo = s->intNo - 32;
    Degf("Int %x!", intNo);
    if (intNo == 0) // timer
    {
      if (TaskManager::get()->isTaskActive())
      {
        // for the moment, only switch task
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
      }
      TaskManager::get()->scheduleNext();
    }
    else if (intNo == 1) // keyboard
      Keyboard::handleInterrupt();
    else if (intNo != 7)
      PANIC("unhandled");
  }
  else if (s->intNo == 0x80) // syscall
  {
    Degf("syscall interrupt %d", s->rax);
    s->rax = sys::handle(*s);
  }

  Degf("Returning to ip %x with sp %x", s->rip, s->rsp);
}
