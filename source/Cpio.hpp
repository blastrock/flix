#ifndef CPIO_HPP
#define CPIO_HPP

#include <cstdint>
#include <vector>
#include <string>

struct File
{
  std::string name;
  std::vector<uint8_t> data;
};

std::vector<File> readArchive(void* data);

#endif
