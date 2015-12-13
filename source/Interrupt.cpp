#include "Interrupt.hpp"
#include "io.hpp"
#include "Debug.hpp"
#include "TaskManager.hpp"
#include "Syscall.hpp"
#include "Keyboard.hpp"
#include "DescTables.hpp"

XLL_LOG_CATEGORY("core/interrupt");

extern "C" void intHandler(InterruptState* s)
{
  Interrupt::handle(s);
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

void Interrupt::handle(InterruptState* s)
{
  xDeb("Interrupt from ip %x with sp %x", s->rip, s->rsp);

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

  auto tm = TaskManager::get();

  if (s->intNo < 32) // CPU exception
  {
    xDeb("Isr %d!", s->intNo);

    switch (s->intNo)
    {
      case 13:
        xDeb("General-Protection Exception (%d)", s->errCode);
        break;
      case 14:
      {
        void* address;
        asm volatile(
            "movq %%cr2, %0"
            :"=r"(address)
            );

        xDeb("Page fault on %p", address);

        if (!(s->errCode & 1) &&
            PageDirectory::getCurrent()->handleFault(address))
          return;

        xDeb("Page was %s", s->errCode & 1 ? "present" : "not present");
        xDeb("Fault on %s", s->errCode & 2 ? "write" : "read");
        xDeb("Access was %s",
            s->errCode & 4 ? "unprivileged" : "privileged");
        xDeb("Fault on %s", s->errCode & 8 ? "fetch" : "execute");
      }
        break;
    }

    xDeb("RIP: %x", s->rip);
    xDeb("RSP: %x", s->rsp);

    {
      ScopedExceptionHandling scope;

      printStackTrace(s->rbp);
    }

    tm->terminateCurrentTask();
  }
  else if (s->intNo < 48) // PIC interrupt
  {
    uint8_t intNo = s->intNo - 32;
    xDeb("Int %x!", intNo);
    if (intNo == 0) // timer
    {
      if (tm->isTaskActive())
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
        tm->saveCurrentTask(context);
      }
      tm->scheduleNext();
    }
    else if (intNo == 1) // keyboard
      Keyboard::handleInterrupt();
    else if (intNo != 7)
      PANIC("unhandled");
  }
  else if (s->intNo == 0x80) // syscall
  {
    xDeb("syscall interrupt %d", s->rax);
    s->rax = sys::handle(*s);
  }

  if (!tm->isTaskActive())
    tm->scheduleNext();

  xDeb("Returning to ip %x with sp %x (int: %s)", s->rip, s->rsp,
      !!(s->rflags & (1 << 9)));
}

static constexpr uint16_t PIC1_CMD = 0x20;
static constexpr uint16_t PIC1_DATA = 0x21;
static constexpr uint16_t PIC2_CMD = 0xA0;
static constexpr uint16_t PIC2_DATA = 0xA1;

void Interrupt::initPic()
{
  // Remap the irq table.
  // init
  io::outb(PIC1_CMD, 0x11);
  io::outb(PIC2_CMD, 0x11);
  // offsets
  io::outb(PIC1_DATA, 0x20);
  io::outb(PIC2_DATA, 0x28);
  // connections
  io::outb(PIC1_DATA, 0x04);
  io::outb(PIC2_DATA, 0x02);
  // environment
  io::outb(PIC1_DATA, 0x01);
  io::outb(PIC2_DATA, 0x01);
  // reset masks
  io::outb(PIC1_DATA, 0x0);
  io::outb(PIC2_DATA, 0x0);
}

template <typename F>
inline static void updateMask(uint8_t nb, F&& op)
{
  uint16_t port;
  uint8_t value;

  if(nb < 8)
    port = PIC1_DATA;
  else
  {
    port = PIC2_DATA;
    nb -= 8;
  }
  value = op(io::inb(port), (1 << nb));
  io::outb(port, value);
}

void Interrupt::mask(uint8_t nb)
{
  updateMask(nb, [](auto x, auto y) { return x | y; });
}

void Interrupt::unmask(uint8_t nb)
{
  updateMask(nb, [](auto x, auto y) { return x & ~y; });
}
