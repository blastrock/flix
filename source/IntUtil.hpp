#ifndef INT_UTIL_HPP
#define INT_UTIL_HPP

#include <cstdint>

class IntUtil
{
  public:
    static char* uintToStr(uint32_t value, uint8_t base);
    static char* ulongToStr(uint64_t value, uint8_t base);
    static char* intToStr(int32_t value, uint8_t base);

  private:
    static char m_buf[32];
};

#endif /* INT_UTIL_HPP */
