#include "Screen.hpp"
#include "DescTables.hpp"
#include "Interrupt.hpp"
#include "Timer.hpp"
#include "Memory.hpp"
#include "Multiboot.hpp"
#include "Debug.hpp"
#include "KHeap.hpp"
#include "PageHeap.hpp"
#include "PageDirectory.hpp"
#include "TaskManager.hpp"
#include "Cpu.hpp"
#include "Syscall.hpp"
#include "Fs.hpp"
#include "Elf.hpp"
#include "io.hpp"
#include "Symbols.hpp"

XLL_LOG_CATEGORY("core/main");

// needed by libkcxx
extern "C" void panic_message(const char* msg)
{
  PANIC(msg);
}

void sleep()
{
  TaskManager::get()->getActiveTask().sh.state = Task::State::Sleeping;
  TaskManager::get()->scheduleNext();
}

void segfault()
{
  *(volatile int*)0xffffffff = 0;
}

void segfault2()
{
  *Symbols::getKernelVTextStart() = 10;
}

uint8_t getCpl()
{
  uint16_t var;
  asm volatile(
      "movw %%cs, %0"
      :"=r"(var)
      );
  return var & 0x3;
}

void loop()
{
  for (unsigned int i = 0; i < 800; ++i)
    xDeb("stuff %d %d", getCpl(), i++);
  //segfault2();
  sys::call(sys::exit);
}

void loop2()
{
  for (unsigned int i = 0; i < 800; ++i)
    xDeb("different stuff %d", i++);
  sys::call(sys::exit);
}

void loop3()
{
  asm volatile ("syscall");
  sys::call(sys::exit);
}

void readwrite()
{
  char buf[100];
  uint64_t sz = sys::call(sys::read, 0, buf, sizeof(buf));
  sys::call(sys::write, 1, "you wrote ", sizeof("you wrote ")-1);
  sys::call(sys::write, 1, buf, sz);
  sys::call(sys::exit);
}

void exec()
{
  xDeb("Opening file");
  auto einode = fs::lookup("/bin/sh");
  if (!einode)
  {
    xErr("failed to open file");
    sys::call(sys::exit);
  }
  auto ehndl = (*einode)->open();
  xDeb("Execing");
  // FIXME exec never returns so hndl leaks
  elf::exec(**ehndl);

  PANIC("init exec failed");
}

static bool ready = false;

class LogHandler : public xll::log::Handler
{
public:
  using char_type = char;
  using traits_type = std::char_traits<char_type>;

  void sputc(char_type ch)
  {
    io::outb(0xe9, ch);
  }
  void sputn(const char_type* s, std::streamsize count)
  {
    while (count--)
      sputc(*s++);
  }

  void beginLog(
      int level,
      const char* category,
      const char* file,
      unsigned int line) override
  {
    (void)level;
    (void)file;
    (void)line;

    if (ready && TaskManager::get()->isTaskActive())
      xll::pnt::writef(*this, "%d: %s: ", TaskManager::get()->getActive()->tid,
          category);
    else
      xll::pnt::writef(*this, "0: %s: ", category);
  }
  void feed(const char* s, std::size_t n) override
  {
    sputn(s, n);
  }
  void endLog() override
  {
    sputc('\n');
  }
};

static LogHandler loghandler;

extern "C" [[noreturn]] int kmain(void* mboot)
{
  Cpu::enableSSEInstructions();

  xll::log::setLevel(xll::log::LEVEL_DEBUG);
  xll::log::setHandler(&loghandler);

  xInf("Booting Flix");

  xInf("Initializing GDT and IDT");
  DescTables::init();

  xInf("Remapping IRQs");
  Interrupt::initPic();

  // first we need a heap (which is preallocated)
  xInf("Heap init");
  KHeap::get().init();

  // second we need to prepare the heap which will be used for pagination
  xInf("PageHeap init");
  getPdPageHeap();

  // third we need pagination
  xInf("Paging init");
  PageDirectory* pd = PageDirectory::initKernelDirectory();
  pd->use();

  // finally we need to keep track of used pages to be able to get new pages
  xInf("Memory init");
  MultibootLoader mbl;
  mbl.prepareMemory(mboot);

  // declare A20 and stuff
  // TODO: map this in kernel memory, could be useful
  Memory::setRangeUsed(0xa0000 / 0x1000, 0xb8000 / 0x1000);
  Memory::setRangeUsed(0xb9000 / 0x1000, 0xe8000 / 0x1000);

  xInf("StackPageHeap init");
  getStackPageHeap();

  auto taskManager = TaskManager::get();

  xInf("Setting up TSS");
  taskManager->setUpTss();

  ready = true;

  xInf("Initializing syscall vector");
  sys::initSysCalls();

  // then we load our module to have a file system
  xInf("Loading module");
  mbl.handle(mboot);

  Timer::init(1);

  Screen::clear();
  Screen::putString("Booting Flix\n");

  {
    Task task = taskManager->newKernelTask();
    task.stack = reinterpret_cast<char*>(0xffffffffa0000000 - 0x4000);
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.pageDirectory.mapRange(task.stack, task.stackTop,
        PageDirectory::ATTR_RW);
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&exec);
    taskManager->addTask(std::move(task));
  }
#if 0
  {
    Task task = taskManager->newKernelTask();
    task.stack = reinterpret_cast<char*>(0xffffffffa0000000 - 0x4000);
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.pageDirectory.mapRange(task.stack, task.stackTop,
        PageDirectory::ATTR_RW);
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&loop);
    taskManager->addTask(std::move(task));
  }
  {
    Task task = taskManager->newKernelTask();
    task.stack = reinterpret_cast<char*>(0xffffffffa0000000 - 0x4000);
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.pageDirectory.mapRange(task.stack, task.stackTop,
        PageDirectory::ATTR_RW);
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&loop2);
    taskManager->addTask(std::move(task));
  }
  {
    Task task = taskManager->newKernelTask();
    task.stack = new char[0x4000];
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = new char[0x4000];
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&sleep);
    taskManager->addTask(std::move(task));
  }
  {
    Task task = taskManager->newKernelTask();
    task.stack = new char[0x4000];
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = new char[0x4000];
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&readwrite);
    taskManager->addTask(std::move(task));
  }
  {
    Task task = taskManager->newKernelTask();
    task.context.rsp = reinterpret_cast<uint64_t>(new char[0x1000])+0x1000;
    task.context.rip = reinterpret_cast<uint64_t>(&loop3);
    taskManager->addTask(std::move(task));
  }
#endif

  xInf("End of kernel");

  taskManager->scheduleNext(); // start a task, never returns

  // this stack will be reused for interrupts when there is no active task

  PANIC("Reached end of main!");
}
