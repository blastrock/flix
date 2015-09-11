#include "Elf.hpp"
#include "ElfFormat.hpp"
#include "TaskManager.hpp"
#include "Debug.hpp"

XLL_LOG_CATEGORY("core/elf");

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
  xDeb("Reading header");
  ElfHeader hdr;
  if (!f.lseek(0, fs::Whence::Begin))
    return false;
  if (!f.read(static_cast<void*>(&hdr), sizeof(hdr)))
    return false;

  if (!checkHeader(hdr))
  {
    const auto& magic = hdr.e_ident;
    xDeb("%x %c%c%c", magic[0], magic[1], magic[2], magic[3]);
    xDeb("Invalid ELF header");
    return false;
  }

  // TODO check arch type

  auto tm = TaskManager::get();
  auto& task = tm->getActiveTask();
  auto& pd = task.pageDirectory;

  xDeb("%d program headers to read", hdr.e_phnum);
  auto currentOffset = hdr.e_phoff;
  for (unsigned int i = 0;
      i < hdr.e_phnum;
      ++i, currentOffset += hdr.e_phentsize)
  {
    if (!f.lseek(currentOffset, fs::Whence::Begin))
      return false;

    xDeb("Reading program header %d", i);
    ElfProgramHeader prgHdr;
    if (!f.read(&prgHdr, sizeof(prgHdr)))
      return false;

    xDeb("Type is 0x%x", prgHdr.p_type);
    if (prgHdr.p_type != PT_LOAD)
      continue;

    xDeb("Mapping segment in memory at %x of size %x",
        prgHdr.p_vaddr, prgHdr.p_memsz);
    for (auto cur = prgHdr.p_vaddr, end = cur + prgHdr.p_memsz;
        cur < end;
        cur += PAGE_SIZE)
      pd.mapPage(reinterpret_cast<char*>(cur),
          PageDirectory::ATTR_RW | PageDirectory::ATTR_PUBLIC);

    xDeb("Zeroing");
    std::memset(reinterpret_cast<char*>(prgHdr.p_vaddr), 0, prgHdr.p_memsz);

    xDeb("Loading segment in file at %x of size %x",
        prgHdr.p_offset, prgHdr.p_filesz);
    // FIXME what should we do if filesz > memsz??
    if (!f.lseek(prgHdr.p_offset, fs::Whence::Begin))
      return false;
    if (!f.read(reinterpret_cast<char*>(prgHdr.p_vaddr),
        prgHdr.p_filesz))
      return false;
  }

  xDeb("Allocating new stack");
  pd.mapRange(reinterpret_cast<void*>(0xffffffff00000000),
      reinterpret_cast<void*>(0xffffffff00004000),
      PageDirectory::ATTR_RW | PageDirectory::ATTR_PUBLIC);
  task.context.rsp = 0xffffffff00004000;

  task.context.rip = reinterpret_cast<uint64_t>(hdr.e_entry);

  // TODO clear registers

  xDeb("Jumping");
  TaskManager::get()->downgradeCurrentTask();
  tm->rescheduleSelf();
}

}
