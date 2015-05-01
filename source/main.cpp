#include "Screen.hpp"
#include "DescTables.hpp"
#include "Timer.hpp"
#include "Memory.hpp"
#include "Multiboot.hpp"
#include "Debug.hpp"
#include "KHeap.hpp"
#include "PageHeap.hpp"
#include "PageDirectory.hpp"
#include "Task.hpp"
#include "Cpu.hpp"
#include "Syscall.hpp"

void write(const char* str)
{
  Degf(str);
}

void segfault()
{
  *(volatile int*)0 = 0;
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
  sys::call(sys::exit);
}

extern "C" int kmain(void* mboot)
{
  Screen::clear();
  Screen::putString("Hello world!\n\nI'm here!\n");

  DescTables::init();

  // first we need a heap (which is preallocated)
  Degf("Heap init");
  KHeap::init();

  // second we need to prepare the heap which will be used for pagination
  Degf("PageHeap init");
  PageHeap::init();

  // third we need pagination
  Degf("Paging init");
  PageDirectory* pd = PageDirectory::initKernelDirectory();
  pd->use();

  // finally we need to keep track of used pages to be able to get new pages
  Degf("Memory init");
  MultibootLoader mbl;
  mbl.handle(mboot);

  Degf("Started!");

  {
    std::string str =
      "This is a very long string which will require a malloc.";

    Degf(str.c_str());

    Degf(typeid(*&str).name());
  }

  //segfault();

  //char* aa = new char[0x300000];
  //for (int i = 0; i < 0x300000; ++i)
  //  aa[i] = 0xaa;
  //delete [] aa;

  //Paging::test(0);

  //Screen::putString("Hello world!\n\nI'm here!\n");

  //asm volatile ("int $0x3");
  //asm volatile ("int $0x4");
  //asm volatile ("int $0x10");
  //asm volatile ("int $0x16");

  auto tm = TaskManager::get();

  Degf("rflags %x", Cpu::rflags());
  {
    Task task = tm->newKernelTask();
    task.context.rsp = reinterpret_cast<uint64_t>(new char[0x1000])+0x1000;
    task.context.rip = reinterpret_cast<uint64_t>(&loop);
    tm->addTask(std::move(task));
  }
  {
    Task task = tm->newKernelTask();
    task.context.rsp = reinterpret_cast<uint64_t>(new char[0x1000])+0x1000;
    task.context.rip = reinterpret_cast<uint64_t>(&loop2);
    tm->addTask(std::move(task));
  }
  {
    Task task = tm->newUserTask();
    task.context.rsp = reinterpret_cast<uint64_t>(new char[0x1000])+0x1000;
    task.context.rip = reinterpret_cast<uint64_t>(&loop3);
    tm->addTask(std::move(task));
  }

  Degf("Setting up TSS");
  TaskManager::setUpTss();

  Degf("Initializing TR");
  DescTables::initTr();

  Timer::init(1);

  Degf("End of kernel");

  TaskManager::get()->scheduleNext();

  asm volatile ("cli");
  Degf("Fatal: reached end of main!");

  // the kernel should never return since the code which called kmain is not
  // mapped anymore in memory
  while (true)
    asm volatile ("hlt");
}
