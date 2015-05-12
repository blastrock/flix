#include "Fs.hpp"

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

}
