#ifndef CPIO_HPP
#define CPIO_HPP

#include <cstdint>
#include <vector>
#include <string>
#include "Fs.hpp"

std::shared_ptr<fs::SuperBlock> readArchive(void* data);

#endif
