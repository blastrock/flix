#include "Cpio.hpp"
#include "Debug.hpp"

struct CpioFile
{
  uint16_t magic;
  uint16_t dev;
  uint16_t ino;
  uint16_t mode;
  uint16_t uid;
  uint16_t gid;
  uint16_t nlink;
  uint16_t rdev;
  uint16_t mtime_hi;
  uint16_t mtime_lo;
  uint16_t namesize;
  uint16_t filesize_hi;
  uint16_t filesize_lo;
} __attribute__((packed));

std::vector<File> readArchive(void* data)
{
  std::vector<File> ret;

  uint8_t* ptr = static_cast<uint8_t*>(data);
  for (;;)
  {
    const CpioFile* curfile = reinterpret_cast<CpioFile*>(ptr);

    if (curfile->magic != 070707)
      PANIC("invalid magic number in cpio file");

    File file;

    ptr += sizeof(CpioFile);

    const uint32_t filesize = curfile->filesize_hi << 16 | curfile->filesize_lo;

    char* name = reinterpret_cast<char*>(ptr);
    file.name = std::string(name, curfile->namesize - 1); // remove the \0

    Degf("Found file %s of size %d", file.name, filesize);

    if (file.name == "TRAILER!!!")
      break;

    ptr += curfile->namesize;
    // round up
    if (curfile->namesize % 2)
      ptr += 1;

    file.data = std::vector<uint8_t>(ptr, ptr + filesize);

    ptr += filesize;
    // round up
    if (filesize % 2)
      ptr += 1;

    ret.emplace_back(std::move(file));
  }

  return ret;
}
