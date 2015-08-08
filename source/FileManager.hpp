#ifndef FILE_MANAGER_HPP
#define FILE_MANAGER_HPP

#include <memory>
#include <vector>

#include <xll/expected.hpp>

#include "Fs.hpp"

class FileManager
{
public:
  FileManager();

  xll::expected<int, fs::IoError> open(const char* name);
  int close(int fd);

  std::shared_ptr<fs::Handle> getHandle(int fd);

private:
  std::vector<std::shared_ptr<fs::Handle>> _files;
};

#endif
