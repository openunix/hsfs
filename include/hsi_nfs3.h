#ifndef __HSI_NFS3_H__
#define __HSI_NFS3_H__ 


#include <sys/stat.h>
#include "hsfs.h"

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
* File read 
*
* @param rw      	the content of the read operation 
*
* @return error number
**/
extern int hsi_nfs3_read(struct hsfs_rw_info* rw);


/**  
* File write 
*
* @param rw      	the content of the write operation 
*
* @return error number
**/
extern int hsi_nfs3_write(struct hsfs_rw_info* rw);


/**
* hsx_nfs3_access
*
* To request whether or not you have the permission to perform an operation.
*
* @param hi the struct hsfs_inode of the node you want to request
* @param mask the permission you want to request
**/
extern int hsi_nfs3_access(struct hsfs_inode *hi, int mask);


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
extern int hsi_nfs3_create(struct hsfs_inode *hi, struct hsfs_inode **newhi, 
							const char *name, mode_t mode);


/**
 *  hsi_nfs3_mknod
 *
 *  To create a special file
 *
 *  @param   parent	the inode of the parent directory
 *  @param   newinode 	the inode of the created file
 *  @param   name 	the name to create
 *  @param   mode	file type and access mode of the file 
 *  @param   rdev	device number of the file: only used for CHR and BLK
 *  * */
extern	int hsi_nfs3_mknod(fuse_req_t req, struct hsfs_inode *parent, struct hsfs_inode **newinode, const char *name,\
				mode_t mode, dev_t rdev);


/**
 * hsi_nfs3_link
 *
 * To create a hard link to a file 
 *
 * @param   ino		the inode of the file
 * @param   newparent	the inode of the new parent
 * @param   newinode	the inode of the linked file 
 * @param   name	the name to of the link to create
 * */
extern 	int hsi_nfs3_link(fuse_req_t req, struct hsfs_inode *ino, struct hsfs_inode *newparent,\
			  struct hsfs_inode **newinode ,  const char *name);
#endif
