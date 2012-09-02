
#include <fuse_lowlevel.h>
#include <rpc/rpc.h>
#include "nfs3.h"

extern (void *)hsx_fuse_mkdir(fuse_req_t req, fuse_ino_t parent, const char* name, mode_t mode);

extern int hsx_fuse_reply_entry(fuse_req_t req, struct hsfs_inode* hsfs_inode);

extern (void *)hsx_fuse_rmdir(fuse_req_t req, fuse_ino_t parent, const char* name);


