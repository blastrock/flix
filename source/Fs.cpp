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
      if (merge && lastiter == iter)
        continue;
      out.push_back(std::string(lastiter, iter));
      lastiter = iter+1;
    }
  }
  return out;
}

std::shared_ptr<Inode> lookup(const std::string& path)
{
  if (path.empty())
    return nullptr;

  if (path[0] != '/')
    return nullptr;

  const auto splitted = split(path, '/', true);
  auto curInode = getRootInode();
  for (const auto& part : splitted)
  {
    curInode = curInode->lookup(part.c_str());
    if (!curInode)
      return nullptr;
  }

  return curInode;
}

}
