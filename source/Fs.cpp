#include "Fs.hpp"

#include <vector>
#include <string>

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

IoExpected<std::shared_ptr<Inode>> lookup(
    const std::shared_ptr<Inode>& inode, const std::string& path)
{
  if (path.empty())
    return inode;

  auto curInode = inode;
  if (path[0] == '/')
    curInode = getRootInode();

  const auto splitted = split(path, '/', true);
  for (const auto& part : splitted)
  {
    auto expCurInode = curInode->lookup(part.c_str());
    if (!expCurInode)
      return xll::make_unexpected(IoError_NotFound{});
    curInode = *expCurInode;
  }

  return curInode;
}

IoExpected<std::shared_ptr<Inode>> lookup(const std::string& path)
{
  if (path.empty())
    return xll::make_unexpected(IoError_InvalidPath{});

  if (path[0] != '/')
    return xll::make_unexpected(IoError_InvalidPath{});

  // TODO use string_view
  return lookup(getRootInode(), path.c_str()+1);
}

}
