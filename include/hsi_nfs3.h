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

/**  
* Read the contents of the symbolic link
*
* @param hi           the struct hsfs_inode of the symbolic link 
* @param nfs_link     the contents of the symbolic link
*
* @return error number
**/
extern  int  hsi _nfs3_readlink( struct hsfs_inode *hi,char *nfs_link)


/**  
* Create a symbolic link
*
* @param parent      the struct hsfs_inode of the parent 
* @param new         the struct hsfs_inode of the new
* @param nfs_link    the contents of the new symbolic link
* @param nfs_name    the name of the new symbolic link
*
* @return error number
**/
extern int  hsi_nfs3_symlink(struct hsfs_inode *parent,struct hsfs_inode *new,const char *nfs_link ,const char *nfs_name)