#include <rpc/rpc.h>
#include <fuse_lowlevel.h>

extern int hsi_nfs3_mkdir(struct hsfs_inode* parent, struct hsfs_inode** new, char* name, mode_t mode);

extern int hsi_nfs3_rmdir(struct hsfs_inode* hsfs_inode, char* name);
