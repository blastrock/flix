#ifndef FS_HPP
#define FS_HPP

#include <cstdint>
#include <cstdlib>
#include <memory>

#include <eggs/variant.hpp>

#include "defs.hpp"

namespace fs
{

enum class Whence
{
  Begin,
  Current,
  End
};

struct IoError_NotFound {};

using IoError = eggs::variant<IoError_NotFound>;

struct Handle {
  virtual ~Handle() {}
  virtual off_t lseek(off_t position, Whence whence)
  {
    (void)position;
    (void)whence;
    return -1;
  }
  virtual off_t read(void* buffer, off_t size)
  {
    (void)buffer;
    (void)size;
    return -1;
  }
  virtual off_t write (const void* buffer, off_t size)
  {
    (void)buffer;
    (void)size;
    return -1;
  }
  //virtual int readdir (void *, filldir_t) = 0;
  //virtual int select (Inode*, File*, int, select_table *) = 0;
  //virtual int ioctl (Inode*, File*, unsigned int, unsigned long) = 0;
  //virtual int mmap (Inode*, File*, struct vm_area_struct *) = 0;
  //virtual void release (Inode*, File*) = 0;
  //virtual int fsync (Inode*, File*) = 0;
  //virtual int fasync (Inode*, File*, int) = 0;
  //virtual int check_media_change (kdev_t dev) = 0;
  //virtual int revalidate (kdev_t dev) = 0;
};

struct Inode {
  uint64_t i_ino;
  //umode_t i_mode;
  //uid_t i_uid;
  //gid_t i_gid;
  //kdev_t i_rdev;
  //loff_t i_size;
  //struct timespec i_atime;
  //struct timespec i_ctime;
  //struct timespec i_mtime;
  //struct super_block *i_sb;
  //struct address_space* i_mapping;
  //struct list_head i_dentry;

  //const struct FileOperations* default_file_ops;

  virtual ~Inode() {}
  //virtual int create(Inode*, const char*, int, int, Inode**) = 0;
  virtual std::shared_ptr<Inode> lookup(const char* name) = 0;
  virtual std::unique_ptr<Handle> open() = 0;
  //virtual int link(Inode*, Inode*, const char*, int) = 0;
  //virtual int unlink(Inode*, const char*, int) = 0;
  //virtual int symlink(Inode*, const char*, int, const char*) = 0;
  //virtual int mkdir(Inode*, const char*, int, int) = 0;
  //virtual int rmdir(Inode*, const char*, int) = 0;
  //virtual int mknod(Inode*, const char*, int, int, int) = 0;
  //virtual int rename(Inode*, const char*, int, Inode*, const char*, int) = 0;
  //virtual int readlink(Inode*, char *, int) = 0;
  //virtual int followlink(Inode*, Inode*, int, int, Inode**) = 0;
  //virtual int readpage(Inode*, struct page *) = 0;
  //virtual int writepage(Inode*, struct page *) = 0;
  //virtual int bmap(Inode*, int) = 0;
  //virtual void truncate(Inode*) = 0;
  //virtual int permission(Inode*, int) = 0;
  //virtual int smap(Inode*, int) = 0;
};

struct SuperBlock {
  virtual ~SuperBlock() {}

  virtual std::shared_ptr<Inode> getRoot() = 0;
  //virtual Inode* allocInode(SuperBlock* sb) = 0;
  //virtual void destroyInode(Inode*) = 0;
  //virtual void read_inode (Inode*) = 0;
  //virtual void dirty_inode (Inode*) = 0;
  //virtual void write_inode (Inode*, int) = 0;
  //virtual void put_inode (Inode*) = 0;
  //virtual void drop_inode (Inode*) = 0;
  //virtual void delete_inode (Inode*) = 0;
  //virtual void put_super (struct super_block *) = 0;
  //virtual void write_super (struct super_block *) = 0;
  //virtual int sync_fs(struct super_block *sb, int wait) = 0;
  //virtual void write_super_lockfs (struct super_block *) = 0;
  //virtual void unlockfs (struct super_block *) = 0;
  //virtual int statfs (struct super_block *, struct statfs *) = 0;
  //virtual int remount_fs (struct super_block *, int *, char *) = 0;
  //virtual void clear_inode (Inode*) = 0;
  //virtual void umount_begin (struct super_block *) = 0;
  //virtual int show_options(struct seq_file *, struct vfsmount *) = 0;
};

void setRoot(std::shared_ptr<SuperBlock> root);
std::shared_ptr<Inode> getRootInode();
std::shared_ptr<Inode> lookup(const std::string& path);

}

#endif
