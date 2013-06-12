#include "Screen.hpp"
#include "DescTables.hpp"
#include "Timer.hpp"
#include "Memory.hpp"
#include "Multiboot.hpp"
#include "Debug.hpp"
#include "KHeap.hpp"
#include "Paging.hpp"

extern "C" int kmain(void* mboot)
{
  Screen::clear();

  DescTables::init();

  //MultibootLoader mbl;
  //mbl.handle(mboot);

  KHeap::init();
  Paging::init();

  Memory::init();

  char* buf = (char*)KHeap::kmalloc(128);

  fDeg() << "buf: " << (void*)buf;

  buf[0] = 'H';
  buf[1] = 'e';
  buf[2] = 'l';
  buf[3] = 'l';
  buf[4] = '\0';
  fDeg() << buf;

  //Paging::test(0);

  //Screen::putString("Hello world!\n\nI'm here!\n");

  asm volatile ("int $0x3");
  //asm volatile ("int $0x4");
  //asm volatile ("int $0x10");
  //asm volatile ("int $0x16");

  //asm volatile ("sti");
  //Timer::init(4);

  fInfo() << "End of kernel";

  return 0xDEADBEEF;
}
