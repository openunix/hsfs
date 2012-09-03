#ifndef __HSI_NFS3_H__
#define __HSI_NFS3_H__ 1

#include <fuse/fuse_lowlevel.h>
#include <sys/stat.h>

/**
 * Make/Remove dir
 *
 * @param hsfs_inode
 * @param name to create
 * @param mode
 * 
 * @return error number
 *
 * */
extern int hsi_nfs3_mkdir(struct hsfs_inode *parent, struct hsfs_inode **new, char *name, mode_t mode);

extern int hsi_nfs3_rmdir(struct hsfs_inode *hsfs_inode, char *name);

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
extern  int  hsi _nfs3_readlink( struct hsfs_inode *hi,char *nfs_link);


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
extern int  hsi_nfs3_symlink(struct hsfs_inode *parent,struct hsfs_inode *new,const char *nfs_link ,const char *nfs_name);

/**
* hsx_nfs3_access
*
* To request whether or not you have the permission to perform an operation.
*
* @param hi the struct hsfs_inode of the node you want to request
* @param mask the permission you want to request
**/
int hsi_nfs3_access(struct hsfs_inode *hi, int mask);


/**
* hsx_nfs3_create
*
* To create a new regular file.
*
* @param hi the struct hsfs_inode of the node you want to create a file
* @param newhi the struct hsfs_inode of the node that you just create
* @param name the name of the file you want to create
*	@param mode the access mode of the file
**/
int hsi_nfs3_create(struct hsfs_inode *hi, struct hsfs_inode **newhi, 
							const char *name, mode_t mode);
#endif
