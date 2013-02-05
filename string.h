#ifndef STRING_H
#define STRING_H

inline void memset(void* addr, int val, int count)
{
  for (char* p = (char*)addr, *end = p+count; p < end; ++p)
    *p = val;
}

inline void memcpy(void* dest, const void* src, int count)
{
  for (char* p1 = (char*)src, *end = p1+count, *p2 = (char*)dest;
      p1 < end; ++p1, ++p2)
    *p2 = *p1;
}

#endif /* STRING_H */
