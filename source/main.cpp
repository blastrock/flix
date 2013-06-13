#include "Screen.hpp"
#include "DescTables.hpp"
#include "Timer.hpp"
#include "Memory.hpp"
#include "Multiboot.hpp"
#include "Debug.hpp"
#include "KHeap.hpp"
#include "Paging.hpp"

void write(const char* str)
{
  fDeg() << str;
}

extern "C" int kmain(void* mboot)
{
  Screen::clear();
  Screen::putString("Hello world!\n\nI'm here!\n");
  return 0;

  DescTables::init();

  //MultibootLoader mbl;
  //mbl.handle(mboot);

  KHeap::init();
  Paging::init();

  Memory::init();

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

  return 0xDEADBEEF;
}
