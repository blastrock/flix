#ifndef DEFS_HPP
#define DEFS_HPP

#include <cstdint>

static constexpr uint8_t MAX_SYMLINK_INDIRECTIONS = 128;
static constexpr uint16_t MAX_PATH_LENGTH = 256;

using mode_t = uint16_t;
using loff_t = uint64_t;
using off_t = uint64_t;
using pid_t = uint32_t;

#endif
