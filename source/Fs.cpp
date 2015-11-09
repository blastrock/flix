#include "Fs.hpp"
#include "Debug.hpp"

#include <vector>
#include <string>

#include <flix/stat.h>

XLL_LOG_CATEGORY("core/vfs/base");

namespace fs
{

static std::shared_ptr<SuperBlock> g_rootSb;

void setRoot(std::shared_ptr<SuperBlock> root)
{
  g_rootSb = root;
}

std::shared_ptr<Inode> getRootInode()
{
  return g_rootSb->getRoot();
}

static std::vector<std::string> split(
    const std::string& str, char val, bool merge)
{
  std::vector<std::string> out;
  auto lastiter = str.begin();
  const auto end = str.end();
  for (auto iter = str.begin(); iter != end; ++iter)
  {
    if (*iter == val)
    {
      if (!merge || lastiter != iter)
        out.push_back(std::string(lastiter, iter));
      lastiter = iter+1;
    }
  }
  if (lastiter != end || !merge)
    out.push_back(std::string(lastiter, end));
  return out;
}

IoExpected<std::shared_ptr<Inode>> lookup(const std::shared_ptr<Inode>& inode,
    const std::string& path,
    LookupOptions options,
    uint8_t indirectionLeft)
{
  if (path.empty())
    return inode;

  xDeb("Looking up %s", path);

  auto curInode = inode;
  if (path[0] == '/')
    curInode = getRootInode();

  const auto splitted = split(path, '/', true);
  decltype(curInode) lastLevel;
  for (const auto& part : splitted)
  {
    lastLevel = curInode;
    xDeb("Searching %s", part);
    auto expCurInode = curInode->lookup(part.c_str());
    if (!expCurInode)
    {
      xDeb("Not found");
      return xll::make_unexpected(IoError_NotFound{});
    }
    curInode = *expCurInode;
  }

  if ((curInode->i_mode & S_IFMT) != S_IFLNK ||
      options == LookupOptions_NoFollowSymlink)
    return curInode;

  if (curInode->i_size > MAX_PATH_LENGTH)
    return xll::make_unexpected(IoError_InvalidPath{});

  xDeb("Following link");

  auto exphandle = curInode->open();
  if (!exphandle)
  {
    xDeb("Couldn't read link");
    return exphandle.get_unexpected();
  }

  auto handle = std::move(*exphandle);
  std::vector<char> buf(curInode->i_size + 1);
  handle->read(&buf[0], curInode->i_size);
  buf[curInode->i_size] = '\0';

  return lookup(lastLevel, &buf[0], options, indirectionLeft - 1);
}

IoExpected<std::shared_ptr<Inode>> lookup(
    const std::string& path, LookupOptions options)
{
  if (path.empty())
    return xll::make_unexpected(IoError_InvalidPath{});

  if (path[0] != '/')
    return xll::make_unexpected(IoError_InvalidPath{});

  // TODO use string_view
  return lookup(getRootInode(), path.c_str()+1, options);
}

}
