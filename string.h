#ifndef STRING_H
#define STRING_H

inline void memset(void* addr, int val, int count)
{
  for (char* p = (char*)addr, *end = p+count; p < end; ++p)
    *p = val;
}

#endif /* STRING_H */
