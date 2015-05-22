#include "Elf.hpp"
#include "Task.hpp"
#include "Debug.hpp"

namespace elf
{

using Elf64_Addr   = uint64_t;
using Elf64_Off    = uint64_t;
using Elf64_Half   = uint16_t;
using Elf64_Word   = uint32_t;
using Elf64_Sword  = int32_t;
using Elf64_Xword  = uint64_t;
using Elf64_Sxword = int64_t;

struct ElfHeader
{
  unsigned char e_ident[16]; /* ELF identification */
  Elf64_Half    e_type;      /* Object file type */
  Elf64_Half    e_machine;   /* Machine type */
  Elf64_Word    e_version;   /* Object file version */
  Elf64_Addr    e_entry;     /* Entry point address */
  Elf64_Off     e_phoff;     /* Program header offset */
  Elf64_Off     e_shoff;     /* Section header offset */
  Elf64_Word    e_flags;     /* Processor-specific flags */
  Elf64_Half    e_ehsize;    /* ELF header size */
  Elf64_Half    e_phentsize; /* Size of program header entry */
  Elf64_Half    e_phnum;     /* Number of program header entries */
  Elf64_Half    e_shentsize; /* Size of section header entry */
  Elf64_Half    e_shnum;     /* Number of section header entries */
  Elf64_Half    e_shstrndx;  /* Section name string table index */
} __attribute__((packed));

struct ElfSectionHeader
{
  Elf64_Word sh_name;       /* Section name */
  Elf64_Word sh_type;       /* Section type */
  Elf64_Xword sh_flags;     /* Section attributes */
  Elf64_Addr sh_addr;       /* Virtual address in memory */
  Elf64_Off sh_offset;      /* Offset in file */
  Elf64_Xword sh_size;      /* Size of section */
  Elf64_Word sh_link;       /* Link to other section */
  Elf64_Word sh_info;       /* Miscellaneous information */
  Elf64_Xword sh_addralign; /* Address alignment boundary */
  Elf64_Xword sh_entsize;   /* Size of entries, if section has table */
} __attribute__((packed));

struct ElfProgramHeader
{
  Elf64_Word p_type;    /* Type of segment */
  Elf64_Word p_flags;   /* Segment attributes */
  Elf64_Off p_offset;   /* Offset in file */
  Elf64_Addr p_vaddr;   /* Virtual address in memory */
  Elf64_Addr p_paddr;   /* Reserved */
  Elf64_Xword p_filesz; /* Size of segment in file */
  Elf64_Xword p_memsz;  /* Size of segment in memory */
  Elf64_Xword p_align;  /* Alignment of segment */
} __attribute__((packed));

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
  f.read(static_cast<void*>(&hdr), 0, sizeof(hdr));

  if (!checkHeader(hdr))
  {
    const auto& magic = hdr.e_ident;
    Degf("%x %c%c%c", magic[0], magic[1], magic[2], magic[3]);
    Degf("Invalid ELF header");
    return false;
  }

  // TODO check arch type

  // TODO loop over segments
  Degf("Reading program header");
  ElfProgramHeader prgHdr;
  f.read(&prgHdr, hdr.e_phoff, sizeof(prgHdr));

  auto tm = TaskManager::get();
  auto& task = tm->getCurrentTask();
  auto& pd = task.pageDirectory;

  Degf("Mapping segment");
  void* vaddr = reinterpret_cast<char*>(prgHdr.p_vaddr);
  pd.mapPage(vaddr, PageDirectory::ATTR_RW | PageDirectory::ATTR_PUBLIC);

  Degf("Loading segment");
  f.read(static_cast<char*>(vaddr), prgHdr.p_offset, prgHdr.p_filesz);

  // reset stack pointer to the top of the stack
  task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);

  task.context.rip = reinterpret_cast<uint64_t>(hdr.e_entry);

  // TODO clear registers

  Degf("Jumping");
  tm->rescheduleSelf();
}

}
