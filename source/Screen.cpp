#include "Screen.hpp"

Position Screen::g_cursor = {0, 0};

extern "C"
{

void panic_message(const char* msg)
{
  Screen::putString(msg);
  Screen::putChar('\n');
  asm("ud2\n");
}

}
