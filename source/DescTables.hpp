#ifndef DESC_TABLES_HPP
#define DESC_TABLES_HPP

#include <cstdint>

class DescTables
{
  public:
    static constexpr unsigned SYSTEM_CS = 0x08;
    static constexpr unsigned SYSTEM_DS = 0x10;
    static constexpr unsigned USER_CS = 0x18;
    static constexpr unsigned USER_DS = 0x20;

    static void init();
    static void initTr();
};

#endif /* DESC_TABLES_HPP */
