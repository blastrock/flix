#include "Cpio.hpp"
#include "Debug.hpp"
#include <flix/stat.h>

XLL_LOG_CATEGORY("core/vfs/cpio");

class CpioFileInode;

class CpioFileHandle : public fs::Handle
{
public:
  CpioFileHandle(std::shared_ptr<CpioFileInode> inode)
    : _file(inode)
    , _pos(0)
  {}

  fs::IoExpected<std::shared_ptr<fs::Inode>> getInode() override;
  fs::IoExpected<off_t> lseek(off_t position, fs::Whence whence) override;
  fs::IoExpected<off_t> read(void* buffer, off_t size) override;

  std::shared_ptr<CpioFileInode> _file;
  off_t _pos;
};

class CpioInode : public fs::Inode
{
public:
  std::string _name;
};

class CpioFileInode : public CpioInode,
                      public std::enable_shared_from_this<CpioFileInode>
{
public:
  fs::IoExpected<std::unique_ptr<fs::Handle>> open() override
  {
    return std::make_unique<CpioFileHandle>(shared_from_this());
  }

  std::vector<uint8_t> _data;
};

fs::IoExpected<std::shared_ptr<fs::Inode>> CpioFileHandle::getInode()
{
  return _file;
}

fs::IoExpected<off_t> CpioFileHandle::lseek(off_t position, fs::Whence whence)
{
  switch (whence)
  {
  case fs::Whence::Begin:
    _pos = position;
    break;
  case fs::Whence::Current:
    _pos += position;
    break;
  case fs::Whence::End:
    _pos = _file->_data.size() + position;
    break;
  }
  // TODO handle _pos < 0 or > size
  xDeb("Seeked to %d", _pos);

  return _pos;
}

fs::IoExpected<off_t> CpioFileHandle::read(void* buf, off_t size)
{
  // TODO handle end of file
  xDeb("Reading %d bytes from %d", size, _pos);
  size = std::min(size, _file->_data.size() - _pos);
  std::memcpy(buf, &_file->_data[_pos], size);
  _pos += size;
  return size;
}

class CpioFolderInode : public CpioInode
{
public:
  fs::IoExpected<std::shared_ptr<fs::Inode>> lookup(const char* name) override
  {
    for (const auto& child : _children)
      if (child->_name == name)
        return child;
    return xll::make_unexpected(fs::IoError_NotFound{});
  }

  std::vector<std::shared_ptr<CpioInode>> _children;
};

class CpioFs : public fs::SuperBlock
{
public:
  CpioFs()
  {
    _root = std::make_shared<CpioFolderInode>();
  }

  virtual std::shared_ptr<fs::Inode> getRoot() override
  {
    return _root;
  }

  std::shared_ptr<CpioFileInode> makeFile(const char* path)
  {
    xDeb("Making file %s", path);
    std::shared_ptr<CpioFolderInode> curInode = _root;
    const char* start = path;
    while(*path != '\0')
    {
      if (*path == '/')
      {
        assert(start < path);

        // TODO use string_view
        const std::string name(start, path);

        auto expChild = curInode->lookup(name.c_str());
        if (expChild)
        {
          xDeb("Found inode %s", name);
          auto child = *expChild;
          if ((child->i_mode & S_IFMT) != S_IFDIR)
          {
            xDeb("not a directory", name);
            return nullptr;
          }
          curInode = std::static_pointer_cast<CpioFolderInode>(*expChild);
        }
        else
        {
          xDeb("Creating inode %s", name);
          auto newFolder = std::make_shared<CpioFolderInode>();
          newFolder->_name = name;
          newFolder->i_mode = S_IFDIR;
          curInode->_children.push_back(newFolder);
          curInode = newFolder;
        }

        start = path+1;
      }

      ++path;
    }

    assert(start < path);

    auto finalInode = std::make_shared<CpioFileInode>();
    finalInode->_name = std::string(start, path);
    xDeb("Final inode %s", finalInode->_name);
    curInode->_children.push_back(finalInode);

    return finalInode;
  }

  std::shared_ptr<CpioFolderInode> _root;
};

struct CpioFileBlock
{
  uint16_t magic;
  uint16_t dev;
  uint16_t ino;
  uint16_t mode;
  uint16_t uid;
  uint16_t gid;
  uint16_t nlink;
  uint16_t rdev;
  uint16_t mtime_hi;
  uint16_t mtime_lo;
  uint16_t namesize;
  uint16_t filesize_hi;
  uint16_t filesize_lo;
} __attribute__((packed));

std::shared_ptr<fs::SuperBlock> readArchive(void* data)
{
  auto cpiofs = std::make_shared<CpioFs>();

  uint8_t* ptr = static_cast<uint8_t*>(data);
  for (;;)
  {
    const CpioFileBlock* curfile = reinterpret_cast<CpioFileBlock*>(ptr);

    if (curfile->magic != 070707)
      PANIC("invalid magic number in cpio file");

    ptr += sizeof(CpioFileBlock);

    const uint32_t filesize = curfile->filesize_hi << 16 | curfile->filesize_lo;

    char* name = reinterpret_cast<char*>(ptr);
    std::string sname(name, curfile->namesize - 1); // remove the \0

    xDeb("Found file %s of size %d", sname, filesize);

    if (sname == "TRAILER!!!")
      break;

    ptr += curfile->namesize;
    // round up
    if (curfile->namesize % 2)
      ptr += 1;

    auto file = cpiofs->makeFile(sname.c_str());
    if (file)
    {
      file->_data = std::vector<uint8_t>(ptr, ptr + filesize);
      file->i_size = filesize;
      file->i_mode = curfile->mode;
    }
    else
      xDeb("Failed to create %s", sname);

    ptr += filesize;
    // round up
    if (filesize % 2)
      ptr += 1;
  }

  return cpiofs;
}
