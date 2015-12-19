#ifndef ELF_HPP
#define ELF_HPP

#include <vector>
#include "Fs.hpp"

namespace elf
{

bool exec(fs::Handle& f, const std::vector<std::string>& args);

}

#endif
