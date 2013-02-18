#ifndef BIT_VECTOR_HPP
#define BIT_VECTOR_HPP

#include "cstdint"
#include "string.h"

class BitVector
{
  public:
    inline void setData(uint64_t size, uint8_t* data);

    inline bool getBit(uint32_t n) const;
    inline void setBit(uint32_t n, bool b);
    inline void fill(bool b);

  private:
    uint64_t m_size;
    uint8_t* m_data;
};

void BitVector::setData(uint64_t size, uint8_t* data)
{
  m_size = size;
  m_data = data;
}

bool BitVector::getBit(uint32_t n) const
{
  return m_data[n/8] & (1 << (n%8));
}

void BitVector::setBit(uint32_t n, bool b)
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
