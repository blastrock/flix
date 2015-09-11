#include "FileManager.hpp"
#include "Screen.hpp"
#include "Debug.hpp"
#include "Tty.hpp"

XLL_LOG_CATEGORY("core/vfs/filemanager");

class NullHandle : public fs::Handle
{};

class StdOut : public fs::Handle
{
public:
  fs::IoExpected<off_t> write(const void* buffer, off_t size) override;
};

fs::IoExpected<off_t> StdOut::write(const void* buffer, off_t size)
{
  auto str = static_cast<const char*>(buffer);
  Screen::putString(str, size);
  return size;
}

class StdIn : public fs::Handle
{
public:
  fs::IoExpected<off_t> read(void* buffer, off_t size) override;
};

fs::IoExpected<off_t> StdIn::read(void* buffer, off_t size)
{
  auto str = static_cast<char*>(buffer);
  return Tty::getCurrent()->readInto(str, size);
}

FileManager::FileManager()
{
  _files.clear();
  _files.reserve(3);
  _files.push_back(std::make_shared<StdIn>());
  _files.push_back(std::make_shared<StdOut>());
  _files.push_back(std::make_shared<StdOut>());
}

xll::expected<int, fs::IoError> FileManager::open(const char* path)
{
  xDeb("Opening %s", path);

  auto ehandle =
    fs::lookup(path).bind([](auto&& inode)
        { return inode->open(); });
  if (!ehandle)
    return ehandle.get_unexpected();
  auto handle = std::move(*ehandle);

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
  xDeb("Closing %d", fd);

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
