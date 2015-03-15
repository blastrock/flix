#include "Interrupt.hpp"
#include "io.hpp"
#include "Debug.hpp"
#include "Task.hpp"

extern "C" void intHandler(InterruptState* s)
{
  InterruptHandler::handle(s);
}

void InterruptHandler::handle(InterruptState* s)
{
  // acknowledge interrupt
  if (s->intNo <= 47)
  {
    if (s->intNo >= 40)
      io::outb(0xA0, 0x20);
    io::outb(0x20, 0x20);
  }

  fDeg() << "INTERRUPT";
  if (s->intNo < 32)
  {
    fDeg() << "Isr " << (int)s->intNo << '!';

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

    fDeg() << "RIP: " << std::hex << s->rip;
    fDeg() << "RSP: " << std::hex << s->rsp;

    uint64_t rbp;
    asm volatile ("mov %%rbp, %0":"=r"(rbp)::);

    printStackTrace(rbp);

    PANIC("exception");
  }
  else
  {
    uint8_t intNo = s->intNo - 32;
    fDeg() << "Int " << (int)intNo << '!';
    if (intNo == 0)
    {
      static int i = 0;
      if (i)
      {
        Task task;
        task.r15 = s->r15;
        task.r14 = s->r14;
        task.r13 = s->r13;
        task.r12 = s->r12;
        task.rbx = s->rbx;
        task.rbp = s->rbp;
        task.r11 = s->r11;
        task.r10 = s->r10;
        task.r9 = s->r9;
        task.r8 = s->r8;
        task.rax = s->rax;
        task.rcx = s->rcx;
        task.rdx = s->rdx;
        task.rsi = s->rsi;
        task.rdi = s->rdi;
        task.rip = s->rip;
        task.cs = s->cs;
        task.rflags = s->rflags;
        task.rsp = s->rsp;
        task.ss = s->ss;
        TaskManager::get()->saveCurrentTask(task);
      }
      else
        i = 1;
      TaskManager::get()->scheduleNext();
    }
  }
}
