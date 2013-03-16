#include "Screen.hpp"
#include "DescTables.hpp"
#include "Timer.hpp"
#include "Multiboot.hpp"
#include "Debug.hpp"

extern "C" int kmain(void* mboot)
{
  Screen::clear();

  DescTables::init();

  MultibootLoader mbl;
  mbl.handle(mboot);

  //Memory::init(mboot);

  //Paging::initialise_paging();

  Screen::putString("Hello world!\n\nI'm here!\n");

  asm volatile ("int $0x3");
  //asm volatile ("int $0x4");
  //asm volatile ("int $0x10");
  //asm volatile ("int $0x16");

  //asm volatile ("sti");
  //Timer::init(4);

  return 0xDEADBEEF;
}
