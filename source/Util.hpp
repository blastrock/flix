#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>

template <typename T>
inline T* ptrAdd(T* ptr, intptr_t val)
{
  return reinterpret_cast<T*>(reinterpret_cast<char*>(ptr)+val);
}

template <typename T1, typename T2>
inline uintptr_t ptrDiff(T1* ptr1, T2* ptr2)
{
  return reinterpret_cast<char*>(ptr1)-reinterpret_cast<char*>(ptr2);
}

/**
 * Align a pointer to val bytes above
 *
 * \example If called with 0x9 and 8, it will return 0x10.
 *
 * \warning val must be a power of two
 */
template <typename T>
inline T* ptrAlignSup(T* ptr, uint8_t val)
{
  return reinterpret_cast<T*>((reinterpret_cast<uintptr_t>(ptr) + val-1)
      & ~(uintptr_t)(val-1));
}

/**
 * Align an integer to val bytes above
 *
 * \example If called with 0x9 and 8, it will return 0x10.
 *
 * \warning val must be a power of two
 */
template <typename T>
inline constexpr T intAlignSup(T base, uint8_t val)
{
  return (base + val-1) & ~(T)(val-1);
}

#endif /* UTIL_HPP */
