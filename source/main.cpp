#include "Screen.hpp"
#include "DescTables.hpp"
#include "Timer.hpp"
#include "Memory.hpp"
#include "Multiboot.hpp"
#include "Debug.hpp"
#include "KHeap.hpp"
#include "PageHeap.hpp"
#include "Paging.hpp"

void write(const char* str)
{
  fDeg() << str;
}

extern "C" int kmain(void* mboot)
{
  Screen::clear();
  Screen::putString("Hello world!\n\nI'm here!\n");

  DescTables::init();

  //MultibootLoader mbl;
  //mbl.handle(mboot);

  // first we need a heap (which is preallocated)
  fDeg() << "Heap init";
  KHeap::init();

  // second we need to keep track of used pages to be able to get new pages
  fDeg() << "Memory init";
  Memory::init();

  // third we need to prepare the heap which will be used for pagination
  fDeg() << "PageHeap init";
  PageHeap::init();

  // finally we need pagination
  fDeg() << "Paging init";
  Paging::init();

  fDeg() << "Started!";

  {
    std::string str =
      "This is a very long string which will require a malloc.";

    fDeg() << str;
  }

  //Paging::test(0);

  //Screen::putString("Hello world!\n\nI'm here!\n");

  //asm volatile ("int $0x3");
  //asm volatile ("int $0x4");
  //asm volatile ("int $0x10");
  //asm volatile ("int $0x16");

  //asm volatile ("sti");
  //Timer::init(4);

  fInfo() << "End of kernel";

  // the kernel should never return since the code which called kmain is not
  // mapped anymore in memory
  while (true)
    asm volatile ("hlt");
}
