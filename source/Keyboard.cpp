#include "Keyboard.hpp"
#include "Tty.hpp"
#include "io.hpp"
#include "Debug.hpp"

XLL_LOG_CATEGORY("input/keyboard");

namespace Keyboard
{

static const char QwertyUpMap[] =
  "\0\033!@#$%^&*()_+\b\t"
  "QWERTYUIOP{}\n\0AS"
  "DFGHJKL:\"~\0|ZXCV"
  "BNM<>?\0*\0 \0";
static const char QwertyLowMap[] =
  "\0\0331234567890-=\b\t"
  "qwertyuiop[]\n\0as"
  "dfghjkl;'`\0\\zxcv"
  "bnm,./\0*\0 \0";

static bool lshift = false;
static bool rshift = false;

void handleInterrupt()
{
  uint8_t in = io::inb(0x60);
  xDeb("Got %x", in);
  switch (in)
  {
  case 0x2A: lshift = true; break;
  case 0xAA: lshift = false; break;
  case 0x36: rshift = true; break;
  case 0xB6: rshift = false; break;
  case 0x1D: // left control
  case 0x38: // left alt
  case 0x3A: // caps lock
    break;
  default:
    if (in < sizeof(QwertyUpMap)/sizeof(*QwertyUpMap))
    {
      if (lshift || rshift)
        Tty::getCurrent()->feedInput(QwertyUpMap[in]);
      else
        Tty::getCurrent()->feedInput(QwertyLowMap[in]);
    }
  }
}

}
