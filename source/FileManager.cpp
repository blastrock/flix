#include "FileManager.hpp"
#include "Screen.hpp"
#include "Debug.hpp"

class NullHandle : public fs::Handle
{};

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
  _files.reserve(3);
  _files.push_back(std::make_shared<NullHandle>());
  _files.push_back(std::make_shared<StdOut>());
  _files.push_back(std::make_shared<NullHandle>());
}

xll::expected<int, fs::IoError> FileManager::open(const char* path)
{
  Degf("opening %s", path);

  auto inode = fs::lookup(path);
  if (!inode)
  {
    Degf("not found");
    return xll::make_unexpected(fs::IoError_NotFound{});
  }
  auto handle = inode->open();
  if (!handle)
  {
    Degf("open failed");
    return xll::make_unexpected(fs::IoError_NotFound{});
  }
  // find a free entry and return it
  for (unsigned i = 0; i < _files.size(); ++i)
    if (!_files[i])
    {
      _files[i] = std::move(handle);
      return i;
    }

  _files.push_back(std::move(handle));
  return _files.size()-1;
}

int FileManager::close(int fd)
{
  Degf("closing %d", fd);

  assert(fd >= 0);
  if (static_cast<unsigned int>(fd) >= _files.size())
    return -1;
  _files[fd] = nullptr;
  return 0;
}

std::shared_ptr<fs::Handle> FileManager::getHandle(int fd)
{
  assert(fd >= 0);
  if (static_cast<unsigned int>(fd) >= _files.size())
    return nullptr;
  return _files[fd];
}
