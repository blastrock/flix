#include "Cpio.hpp"
#include "Debug.hpp"

class CpioFileInode;

class CpioFileHandle : public fs::Handle
{
public:
  CpioFileHandle(CpioFileInode* inode)
    : _file(inode)
  {}

  uint64_t read(char* buf, uint64_t pos, uint64_t size) override;

  CpioFileInode* _file;
};

class CpioInode : public fs::Inode
{
public:
  std::string _name;
};

class CpioFileInode : public CpioInode
{
public:
  std::shared_ptr<fs::Inode> lookup(const char*) override
  {
    PANIC("No lookup for files");
  }
  std::unique_ptr<fs::Handle> open() override
  {
    return std::make_unique<CpioFileHandle>(this);
  }

  std::vector<uint8_t> _data;
};

uint64_t CpioFileHandle::read(char* buf, uint64_t pos, uint64_t size)
{
  size = std::min(size, _file->_data.size() - pos);
  std::memcpy(buf, &_file->_data[pos], size);
  return size;
}

class CpioFolderInode : public CpioInode
{
public:
  std::shared_ptr<fs::Inode> lookup(const char* name) override
  {
    for (const auto& child : _children)
      if (child->_name == name)
        return child;
    return nullptr;
  }
  std::unique_ptr<fs::Handle> open() override
  {
    return nullptr;
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
    Degf("Making file %s", path);
    std::shared_ptr<CpioFolderInode> curInode = _root;
    const char* start = path;
    while(*path != '\0')
    {
      if (*path == '/')
      {
        assert(start < path);

        auto newFolder = std::make_shared<CpioFolderInode>();
        newFolder->_name = std::string(start, path);
        Degf("Inode %s", newFolder->_name);
        curInode->_children.push_back(newFolder);
        curInode = newFolder;
        start = path+1;
      }

      ++path;
    }

    assert(start < path);

    auto finalInode = std::make_shared<CpioFileInode>();
    finalInode->_name = std::string(start, path);
    Degf("Final inode %s", finalInode->_name);
    curInode->_children.push_back(finalInode);

    return finalInode;
  }

  std::shared_ptr<CpioFolderInode> _root;
};

struct CpioFile
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
    const CpioFile* curfile = reinterpret_cast<CpioFile*>(ptr);

    if (curfile->magic != 070707)
      PANIC("invalid magic number in cpio file");

    ptr += sizeof(CpioFile);

    const uint32_t filesize = curfile->filesize_hi << 16 | curfile->filesize_lo;

    char* name = reinterpret_cast<char*>(ptr);
    std::string sname(name, curfile->namesize - 1); // remove the \0

    Degf("Found file %s of size %d", sname, filesize);

    if (sname == "TRAILER!!!")
      break;

    ptr += curfile->namesize;
    // round up
    if (curfile->namesize % 2)
      ptr += 1;

    ptr += filesize;
    // round up
    if (filesize % 2)
      ptr += 1;

    auto file = cpiofs->makeFile(sname.c_str());
    file->_data = std::vector<uint8_t>(ptr, ptr + filesize);
  }

  return cpiofs;
}
