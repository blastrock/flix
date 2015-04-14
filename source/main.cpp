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

void write(const char* str)
{
  Degf(str);
}

void segfault()
{
  *(volatile int*)0 = 0;
}

void loop()
{
  unsigned int i = 0;
  while (true)
    Degf("stuff %d", i++);
}

void loop2()
{
  unsigned int i = 0;
  while (true)
    Degf("different stuff %d", i++);
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

  {
    Task task;
    std::memset(&task, 0, sizeof(task));
    task.rsp = reinterpret_cast<uint64_t>(new char[0x10000]);
    task.rip = reinterpret_cast<uint64_t>(&loop);
    task.cs = 0x08;
    task.ss = 0x10;
    task.rflags = Cpu::rflags();
    tm->addTask(task);
  }
  {
    Task task;
    std::memset(&task, 0, sizeof(task));
    task.rsp = reinterpret_cast<uint64_t>(new char[0x10000]);
    task.rip = reinterpret_cast<uint64_t>(&loop2);
    task.cs = 0x08;
    task.ss = 0x10;
    task.rflags = Cpu::rflags();
    tm->addTask(task);
  }

  Timer::init(4);
  asm volatile ("sti");

  Degf("End of kernel");

  // the kernel should never return since the code which called kmain is not
  // mapped anymore in memory
  while (true)
    asm volatile ("hlt");
}
