#include "Elf.hpp"
#include "ElfFormat.hpp"
#include "TaskManager.hpp"
#include "Debug.hpp"

namespace elf
{

bool checkHeader(const ElfHeader& hdr)
{
  const unsigned char* magic = hdr.e_ident;
  return magic[0] == 0x7f && magic[1] == 'E' && magic[2] == 'L'
    && magic[3] == 'F';
}

bool exec(fs::Handle& f)
{
  Degf("Reading header");
  ElfHeader hdr;
    f.lseek(0, fs::Whence::Begin);
  f.read(static_cast<void*>(&hdr), sizeof(hdr));

  if (!checkHeader(hdr))
  {
    const auto& magic = hdr.e_ident;
    Degf("%x %c%c%c", magic[0], magic[1], magic[2], magic[3]);
    Degf("Invalid ELF header");
    return false;
  }

  // TODO check arch type

  auto tm = TaskManager::get();
  auto& task = tm->getActiveTask();
  auto& pd = task.pageDirectory;

  Degf("%d program headers to read", hdr.e_phnum);
  auto currentOffset = hdr.e_phoff;
  for (unsigned int i = 0;
      i < hdr.e_phnum;
      ++i, currentOffset += hdr.e_phentsize)
  {
    f.lseek(currentOffset, fs::Whence::Begin);

    Degf("Reading program header %d", i);
    ElfProgramHeader prgHdr;
    f.read(&prgHdr, sizeof(prgHdr));

    Degf("Type is 0x%x", prgHdr.p_type);
    if (prgHdr.p_type != PT_LOAD)
      continue;

    Degf("Mapping segment in memory at %x of size %x",
        prgHdr.p_vaddr, prgHdr.p_memsz);
    for (auto cur = prgHdr.p_vaddr, end = cur + prgHdr.p_memsz;
        cur < end;
        cur += PAGE_SIZE)
      pd.mapPage(reinterpret_cast<char*>(cur),
          PageDirectory::ATTR_RW | PageDirectory::ATTR_PUBLIC);

    Degf("Zeroing");
    std::memset(reinterpret_cast<char*>(prgHdr.p_vaddr), 0, prgHdr.p_memsz);

    Degf("Loading segment in file at %x of size %x",
        prgHdr.p_offset, prgHdr.p_filesz);
    // FIXME what should we do if filesz > memsz??
    f.lseek(prgHdr.p_offset, fs::Whence::Begin);
    f.read(reinterpret_cast<char*>(prgHdr.p_vaddr),
        prgHdr.p_filesz);
  }

  Degf("Allocating new stack");
  pd.mapRange(reinterpret_cast<void*>(0xffffffff00000000),
      reinterpret_cast<void*>(0xffffffff00004000),
      PageDirectory::ATTR_RW | PageDirectory::ATTR_PUBLIC);
  task.context.rsp = 0xffffffff00004000;

  task.context.rip = reinterpret_cast<uint64_t>(hdr.e_entry);

  // TODO clear registers

  Degf("Jumping");
  TaskManager::get()->downgradeCurrentTask();
  tm->rescheduleSelf();
}

}
