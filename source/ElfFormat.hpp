#ifndef ELF_FORMAT_HPP
#define ELF_FORMAT_HPP

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

static constexpr Elf64_Word PT_NULL = 0;
static constexpr Elf64_Word PT_LOAD = 1;
static constexpr Elf64_Word PT_DYNAMIC = 2;
static constexpr Elf64_Word PT_INTERP = 3;
static constexpr Elf64_Word PT_NOTE = 4;
static constexpr Elf64_Word PT_SHLIB = 5;
static constexpr Elf64_Word PT_PHDR = 6;
static constexpr Elf64_Word PT_LOPROC = 0x70000000;
static constexpr Elf64_Word PT_HIPROC = 0x7fffffff;

}

#endif /* ELF_FORMAT_HPP */
