// main.c -- Defines the C-code kernel entry point, calls initialisation routines.
// Made for JamesM's tutorials

#include "Screen.hpp"
#include "Singleton.hpp"

extern "C" int main(struct multiboot *mboot_ptr)
{
  // All our initialisation calls will go in here.

  Screen* s = Singleton<Screen>::get();
  s->putChar({0, 0}, 'h');
  s->putChar({1, 0}, 'e');
  s->putChar({2, 0}, 'l');
  s->putChar({3, 0}, 'l');
  s->putChar({4, 0}, 'x');

  return 0xDEADBEEF;
}
