#include "FileManager.hpp"
#include "Screen.hpp"

class StdOut : public fs::Handle
{
public:
  virtual off_t write(const void* buffer, off_t size) override;
};

off_t StdOut::write(const void* buffer, off_t size)
{
  const char* str = static_cast<const char*>(buffer);
  Screen::putString(str, size);
  return size;
}

FileManager::FileManager()
{
  _files.clear();
  _files.resize(3);
  _files[1] = std::make_shared<StdOut>();
}

xll::expected<int, fs::IoError> FileManager::open(const char* path)
{
  auto inode = fs::lookup(path);
  if (!inode)
    return xll::make_unexpected(fs::IoError_NotFound{});
  // find a free entry and return it
  for (unsigned i = 0; i < _files.size(); ++i)
    if (!_files[i])
    {
      _files[i] = inode->open();
      return i;
    }

  _files.push_back(inode->open());
  return _files.size()-1;
}

std::shared_ptr<fs::Handle> FileManager::getHandle(int fd)
{
  assert(fd >= 0);
  if (static_cast<unsigned int>(fd) >= _files.size())
    return nullptr;
  return _files[fd];
}
