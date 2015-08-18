#include "Keyboard.hpp"
#include "Tty.hpp"
#include "io.hpp"

namespace Keyboard
{

static const char QwertyMap[] =
  "\0\0001234567890-=\0\0"
  "QWERTYUIOP[]\n\0AS"
  "DFGHJKL;'`\0\\ZXCV"
  "BNM,./\0*\0 \0";

void handleInterrupt()
{
  uint8_t b = io::inb(0x60);
  if (b < sizeof(QwertyMap)/sizeof(*QwertyMap))
    Tty::getCurrent()->feedInput(QwertyMap[b]);
}

}
