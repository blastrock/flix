#include "DescTables.hpp"
#include "Interrupt.hpp"
#include "Timer.hpp"
#include "Memory.hpp"
#include "Multiboot.hpp"
#include "Debug.hpp"
#include "KHeap.hpp"
#include "PageHeap.hpp"
#include "PageDirectory.hpp"
#include "TaskManager.hpp"
#include "Cpu.hpp"
#include "Syscall.hpp"
#include "Fs.hpp"
#include "Elf.hpp"
#include "io.hpp"
#include "Symbols.hpp"

XLL_LOG_CATEGORY("core/kmain");

// needed by libkcxx
extern "C" void panic_message(const char* msg)
{
  PANIC(msg);
}

[[noreturn]] void _main();

static bool ready = false;

class LogHandler : public xll::log::Handler
{
public:
  using char_type = char;
  using traits_type = std::char_traits<char_type>;

  void sputc(char_type ch)
  {
    io::outb(0xe9, ch);
  }
  void sputn(const char_type* s, std::streamsize count)
  {
    while (count--)
      sputc(*s++);
  }

  void beginLog(
      int level,
      const char* category,
      const char* file,
      unsigned int line) override
  {
    (void)level;
    (void)file;
    (void)line;

    if (ready && TaskManager::get()->isTaskActive())
      xll::pnt::writef(*this, "%d: %s: ", TaskManager::get()->getActive()->tid,
          category);
    else
      xll::pnt::writef(*this, "0: %s: ", category);
  }
  void feed(const char* s, std::size_t n) override
  {
    sputn(s, n);
  }
  void endLog() override
  {
    sputc('\n');
  }
};

static LogHandler loghandler;

extern "C" [[noreturn]] void kmain(void* mboot)
{
  Cpu::enableSSEInstructions();

  xll::log::setLevel(xll::log::LEVEL_DEBUG);
  xll::log::setHandler(&loghandler);

  xInf("Booting Flix");

  xInf("Initializing GDT and IDT");
  DescTables::init();

  xInf("Remapping IRQs");
  Interrupt::initPic();

  // first we need a heap (which is preallocated)
  xInf("Heap init");
  KHeap::get().init();

  // second we need to prepare the heap which will be used for pagination
  xInf("PageHeap init");
  getPdPageHeap();

  // third we need pagination
  xInf("Paging init");
  PageDirectory* pd = PageDirectory::initKernelDirectory();
  pd->use();

  // finally we need to keep track of used pages to be able to get new pages
  xInf("Memory init");
  MultibootLoader mbl;
  mbl.prepareMemory(mboot);

  // declare A20 and stuff
  // TODO: map this in kernel memory, could be useful
  Memory::completeRangeUsed(0xa0000 / 0x1000, 0xb8000 / 0x1000);
  Memory::completeRangeUsed(0xb9000 / 0x1000, 0xe8000 / 0x1000);

  xInf("StackPageHeap init");
  getStackPageHeap();

  auto taskManager = TaskManager::get();

  xInf("Setting up TSS");
  taskManager->setUpTss();

  ready = true;

  xInf("Initializing syscall vector");
  sys::initSysCalls();

  // then we load our module to have a file system
  xInf("Loading module");
  mbl.handle(mboot);

  Timer::init(1);

  xInf("Calling _main");
  _main();

  PANIC("Reached end of kmain!");
}
