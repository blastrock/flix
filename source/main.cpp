#include "Screen.hpp"
#include "DescTables.hpp"
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
#include "Symbols.hpp"

// needed by libkcxx
extern "C" void panic_message(const char* msg)
{
  PANIC(msg);
}

void sleep()
{
  TaskManager::get()->getActiveTask().state = Task::State::Sleeping;
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
    Degf("stuff %d %d", getCpl(), i++);
  //segfault2();
  sys::call(sys::exit);
}

void loop2()
{
  for (unsigned int i = 0; i < 800; ++i)
    Degf("different stuff %d", i++);
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
  Degf("Opening file");
  auto einode = fs::getRootInode()->lookup("init");
  auto ehndl = (*einode)->open();
  Degf("Execing");
  // FIXME exec never returns so hndl leaks
  elf::exec(**ehndl);

  PANIC("init exec failed");
}

void enableSSEInstructions()
{
  asm volatile (
      "mov %%cr4, %%rax\n"
      "btsq $9, %%rax\n"
      "mov %%rax, %%cr4\n"
      :
      :
      :"rax");
}

extern "C" [[noreturn]] int kmain(void* mboot)
{
  enableSSEInstructions();

  Degf("Booting Flix");

  Degf("Initializing GDT and IDT");
  DescTables::init();

  // first we need a heap (which is preallocated)
  Degf("Heap init");
  KHeap::get().init();

  // second we need to prepare the heap which will be used for pagination
  Degf("PageHeap init");
  PageHeap::get().init();

  // third we need pagination
  Degf("Paging init");
  PageDirectory* pd = PageDirectory::initKernelDirectory();
  pd->use();

  // finally we need to keep track of used pages to be able to get new pages
  Degf("Memory init");
  MultibootLoader mbl;
  mbl.prepareMemory(mboot);

  auto taskManager = TaskManager::get();

  Degf("Setting up TSS");
  taskManager->setUpTss();

  Degf("Initializing syscall vector");
  sys::initSysCalls();

  // then we load our module to have a file system
  Degf("Loading module");
  mbl.handle(mboot);

  Timer::init(1);

  Screen::clear();
  Screen::putString("Booting Flix\n");

  {
    Task task = taskManager->newKernelTask();
    task.stack = new char[0x4000];
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = new char[0x4000];
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&exec);
    taskManager->addTask(std::move(task));
  }
  //{
  //  Task task = taskManager->newKernelTask();
  //  task.stack = new char[0x4000];
  //  task.stackTop = task.stack + 0x4000;
  //  task.kernelStack = new char[0x4000];
  //  task.kernelStackTop = task.kernelStack + 0x4000;
  //  task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
  //  task.context.rip = reinterpret_cast<uint64_t>(&sleep);
  //  taskManager->addTask(std::move(task));
  //}
  {
    Task task = taskManager->newKernelTask();
    task.stack = new char[0x4000];
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = new char[0x4000];
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&loop);
    taskManager->addTask(std::move(task));
  }
  {
    Task task = taskManager->newKernelTask();
    task.stack = new char[0x4000];
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = new char[0x4000];
    task.kernelStackTop = task.kernelStack + 0x4000;
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
    task.context.rip = reinterpret_cast<uint64_t>(&readwrite);
    taskManager->addTask(std::move(task));
  }
#if 0
  {
    Task task = taskManager->newKernelTask();
    task.context.rsp = reinterpret_cast<uint64_t>(new char[0x1000])+0x1000;
    task.context.rip = reinterpret_cast<uint64_t>(&loop3);
    taskManager->addTask(std::move(task));
  }
#endif

  Degf("End of kernel");

  taskManager->scheduleNext(); // start a task, never returns

  // TODO this "task"'s stack now becomes useless, can we recycle it someway?

  PANIC("Reached end of main!");
}
