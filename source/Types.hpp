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
/// Number of meaningful bits for virtual addressing
static constexpr unsigned VIRT_ADDRESS_BITS = 48;
/// Mask of meaningful virtual addressing bits
static constexpr uintptr_t VIRT_ADDRESS_MASK =
    (static_cast<uintptr_t>(1) << VIRT_ADDRESS_BITS) - 1;
/// Number of meaningful bits for physical addressing
static constexpr unsigned PHYS_ADDRESS_BITS = 52;
/// Mask of meaningful address bits
static constexpr physaddr_t PHYS_ADDRESS_MASK =
    (static_cast<physaddr_t>(1) << PHYS_ADDRESS_BITS) - 1;
/// Invalid page index
static constexpr page_t INVALID_PAGE =
    (static_cast<page_t>(-1) & PHYS_ADDRESS_MASK) >> LOG_PAGE_SIZE;
/// Invalid physical address
static constexpr page_t INVALID_PHYS = static_cast<physaddr_t>(-1);

#endif
