#ifndef INTTYPES_HPP
#define INTTYPES_HPP

typedef char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef long long i64;
typedef unsigned long long u64;

static_assert(sizeof(i8) == 1, "sizeof(i8) != 1");
static_assert(sizeof(i16) == 2, "sizeof(i16) != 2");
static_assert(sizeof(i32) == 4, "sizeof(i32) != 4");

#endif /* INTTYPES_HPP */
