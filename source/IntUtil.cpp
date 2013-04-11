#include "IntUtil.hpp"

char IntUtil::m_buf[32];

char* IntUtil::uintToStr(uint32_t value, uint8_t base)
{
  m_buf[sizeof(m_buf)-1] = '\0';

  char* ptr;
  for (ptr = m_buf+sizeof(m_buf)-2; ptr >= m_buf; --ptr)
  {
    *ptr = value % base;
    value = value / base;

    if (*ptr >= 10)
      *ptr += 'A' - 10;
    else
      *ptr += '0';

    if (!value)
      break;
  }

  return ptr;
}

char* IntUtil::ulongToStr(uint64_t value, uint8_t base)
{
  m_buf[sizeof(m_buf)-1] = '\0';

  char* ptr;
  for (ptr = m_buf+sizeof(m_buf)-2; ptr >= m_buf; --ptr)
  {
    *ptr = value % base;
    value = value / base;

    if (*ptr >= 10)
      *ptr += 'A' - 10;
    else
      *ptr += '0';

    if (!value)
      break;
  }

  return ptr;
}

char* IntUtil::intToStr(int32_t value, uint8_t base)
{
  m_buf[sizeof(m_buf)-1] = '\0';

  bool neg = value < 0;
  if (neg)
    value = -value;

  char* ptr;
  for (ptr = m_buf+sizeof(m_buf)-2; ptr >= m_buf; --ptr)
  {
    *ptr = value % base;
    value = value / base;

    if (*ptr >= 10)
      *ptr += 'A' - 10;
    else
      *ptr += '0';

    if (!value)
      break;
  }

  if (neg && ptr > m_buf)
    *--ptr = '-';

  return ptr;
}
