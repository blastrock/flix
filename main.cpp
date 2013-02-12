// main.c -- Defines the C-code kernel entry point, calls initialisation routines.
// Made for JamesM's tutorials

#include "Screen.hpp"
#include "DescTables.hpp"
#include "Timer.hpp"
#include "Paging.hpp"

extern "C" int kmain(struct multiboot *mboot_ptr)
{
  Screen::clear();

  // All our initialisation calls will go in here.
  DescTables::init();
  Paging::initialise_paging();

  //Screen::putChar({0, 0}, 'h');
  //Screen::putChar({1, 0}, 'e');
  //Screen::putChar({2, 0}, 'l');
  //Screen::putChar({3, 0}, 'l');
  //Screen::putChar({4, 0}, 'x');

  Screen::putHex((int)&kmain);
  Screen::putString("Hello world!\n\nI'm here!\n");
  //Screen::putInt((int)&kmain);

  asm volatile ("int $0x3");
  //asm volatile ("int $0x4");
  //asm volatile ("int $0x10");
  //asm volatile ("int $0x16");

  //asm volatile ("sti");
  Timer::init(4);


  return 0xDEADBEEF;
}
