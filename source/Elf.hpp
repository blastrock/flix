#ifndef ELF_HPP
#define ELF_HPP

#include "Fs.hpp"
#include "PageDirectory.hpp"

namespace elf
{

bool exec(fs::Handle& f, PageDirectory& pd);

}

#endif
