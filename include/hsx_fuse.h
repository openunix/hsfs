
#include <fuse_lowlevel.h>
#include <rpc/rpc.h>
#include "nfs3.h"

extern (void *)hsx_fuse_mkdir(fuse_req_t req, fuse_ino_t parent, const char* name, mode_t mode);

extern int hsx_fuse_reply_entry(fuse_req_t req, struct hsfs_inode* hsfs_inode);

extern (void *)hsx_fuse_rmdir(fuse_req_t req, fuse_ino_t parent, const char* name);


/**
* Remove a file
*
* If the file's inode's lookup count is non-zero, the file
* system is expected to postpone any removal of the inode
* until the lookup count reaches zero (see description of the
* forget function).
*
* Valid replies:
*   fuse_reply_err
*
* @param req request handle
* @param parent inode number of the parent directory
* @param name to remove
**/
extern void hsx_fuse_unlink(fuse_req_t req, fuse_ino_t parent, const char *name);

/** 
 * Rename a file
 *
 * If the target exists it should be atomically replaced. If
 * the target's inode's lookup count is non-zero, the file
 * system is expected to postpone any removal of the inode
 * until the lookup count reaches zero (see description of the
 * forget function).
 *
 * Valid replies:
 *   fuse_reply_err
 *
 * @param req request handle
 * @param parent inode number of the old parent directory
 * @param name old name
 * @param newparent inode number of the new parent directory
 * @param newname new name
 **/
extern void hsx_fuse_rename(fuse_req_t req, fuse_ino_t parent, const char *name, 
		fuse_ino_t newparent, const char *newname)

