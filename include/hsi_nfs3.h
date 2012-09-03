#include <rpc/rpc.h>
#include <fuse_lowlevel.h>

extern int hsi_nfs3_mkdir(struct hsfs_inode* parent, struct hsfs_inode** new, char* name, mode_t mode);

extern int hsi_nfs3_rmdir(struct hsfs_inode* hsfs_inode, char* name);

/**  
* Remove a file
*
* @param hsfs_inode
* @param name to remove
*
* @return error number
**/
extern int hsi_nfs3_remove(struct hsfs_inode *hi, const char *name);

/**
* Rename a file
*
* @param source hsfs_inode
* @param source name
* @param target hsfs_inode
* @param target name
*
* @return error number
**/
extern int hsi_nfs3_rename(struct hsfs_inode *hi, const char *name,
 		struct hsfs_inode *newhi, const char *newname);

/**  
* Change nfs3 status number to fuse error number
*
* @param nfs3 status number
*
* @return fuse error number
**/
extern int hsi_nfs3_stat_to_errno(int stat);

