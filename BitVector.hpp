#ifndef BIT_VECTOR_HPP
#define BIT_VECTOR_HPP

#include "inttypes.hpp"
#include "string.h"

class BitVector
{
  public:
    inline void setData(u64 size, u8* data);

    inline bool getBit(u32 n) const;
    inline void setBit(u32 n, bool b);
    inline void fill(bool b);

  private:
    u64 m_size;
    u8* m_data;
};

void BitVector::setData(u64 size, u8* data)
{
  m_size = size;
  m_data = data;
}

bool BitVector::getBit(u32 n) const
{
  return m_data[n/8] & (1 << (n%8));
}

void BitVector::setBit(u32 n, bool b)
{
  if (b)
    m_data[n/8] |= (1 << (n%8));
  else
    m_data[n/8] &= ~(1 << (n%8));
}

void BitVector::fill(bool b)
{
  if (b)
    memset(m_data, 0xFF, m_size);
  else
    memset(m_data, 0x00, m_size);
}

#endif /* BIT_VECTOR_HPP */
