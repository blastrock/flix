// main.c -- Defines the C-code kernel entry point, calls initialisation routines.
// Made for JamesM's tutorials

#include "Screen.hpp"
#include "Singleton.hpp"

extern "C" int main(struct multiboot *mboot_ptr)
{
  // All our initialisation calls will go in here.

  Screen::clear();
  //Screen::putChar({0, 0}, 'h');
  //Screen::putChar({1, 0}, 'e');
  //Screen::putChar({2, 0}, 'l');
  //Screen::putChar({3, 0}, 'l');
  //Screen::putChar({4, 0}, 'x');

  Screen::putString("Hello world!\n\nI'm here!");

  return 0xDEADBEEF;
}
