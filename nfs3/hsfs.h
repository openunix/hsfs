
#ifndef __HSFS_FUSE_H__
#define __HSFS_FUSE_H__ 1

#define FUSE_USE_VERSION 26

#include <fuse_lowlevel.h>

extern void hsx_fuse_init(void *, struct fuse_conn_info *);
extern void hsx_fuse_destroy(void *);

#endif
