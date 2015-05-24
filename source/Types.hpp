#ifndef TYPES_HPP
#define TYPES_HPP

#include <stdint.h>

/// A physical address in memory
using physaddr_t = uintptr_t;
/// A page index in memory (physical address / page size)
using page_t = uintptr_t;

/// log2(PAGE_SIZE)
static constexpr unsigned LOG_PAGE_SIZE = 12;
/// Size of a page in bytes
static constexpr unsigned PAGE_SIZE = 1 << LOG_PAGE_SIZE;
/// Invalid page index
static constexpr page_t INVALID_PAGE = static_cast<page_t>(-1);

#endif
