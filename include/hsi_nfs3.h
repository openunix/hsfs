#ifndef __HSI_NFS3_H__
#define __HSI_NFS3_H__

#include "hsfs.h"

/**
 * @brief make a directory
 *
 * @param parent[in] the information of the source parent directory
 * @param name[in] directory name
 * @param new[in] the information of the target directory
 * @param mode[in] the mode of the target directory
 *
 * @return error number
 **/
extern int hsi_nfs3_mkdir(struct hsfs_inode *parent, struct hsfs_inode **new,
	       		const char *name, mode_t mode);
/**
 * @brief remove a directory
 *
 * @param parent[in] the information of the source parent directory
 * @param name[in] directory name
 *
 * @return error number
 **/
extern int hsi_nfs3_rmdir(struct hsfs_inode *parent, const char *name);

/**
 * @brief Remove a file
 *
 * @param parent[in] the information of the parent directory
 * @param name[in] to remove
 *
 * @return error number
 **/
extern int hsi_nfs3_unlink(struct hsfs_inode *parent, const char *name);

/**
 * @brief Rename a file
 *
 * @param parent[in] the information of the source parent directory
 * @param name[in] source name
 * @param newparent[in] the information of the target parent directory
 * @param newname[in] target name
 *
 * @return error number
 **/
extern int hsi_nfs3_rename(struct hsfs_inode *parent, const char *name,
			   struct hsfs_inode *newparent, const char *newname);

/**
 * @brief Change nfs3 status number to system error number
 *
 * @param stat[in] nfs3 status number
 *
 * @return error number
 **/
extern int hsi_nfs3_stat_to_errno(int stat);

/**
 * @brief Change rpc status number to system error number
 *
 * @param clntp[in] rpc client handle
 *
 * @return error number
 **/
extern int hsi_rpc_stat_to_errno(CLIENT *clntp);


/**
 * Read the contents of the symbolic link
 *
 * @param hi           the struct hsfs_inode of the symbolic link 
 * @param nfs_link     the contents of the symbolic link
 *
 * @return error number
 **/

extern int hsi_nfs3_readlink(struct hsfs_inode *hi, char **nfs_link);


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
extern int hsi_nfs3_symlink(struct hsfs_inode *parent, struct hsfs_inode **new, 
			    const char *nfs_link, const char *nfs_name);

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
 * hsi_nfs3_access
 *
 * To request whether or not you have the permission to perform an operation.
 *
 * @param hi the struct hsfs_inode of the node you want to request
 * @param mask the permission you want to request
 **/
extern int hsi_nfs3_access(struct hsfs_inode *hi, int mask);


/**
 * hsi_nfs3_create
 *
 * To create a new regular file.
 *
 * @param hi the struct hsfs_inode of the node you want to create a file
 * @param newhi the struct hsfs_inode of the node that you just create
 * @param name the name of the file you want to create
 *	@param mode the access mode of the file
 */
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
extern	int hsi_nfs3_mknod(struct hsfs_inode *parent, struct hsfs_inode 
		  	**newinode, const char *name,mode_t mode, dev_t rdev);

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
extern 	int hsi_nfs3_link(struct hsfs_inode *ino, struct hsfs_inode *newparent, 
			struct hsfs_inode **newinode ,  const char *name);

/**
 * hsi_nfs3_lookup:		Look up a directory entry by name
 *
 * @param parent[IN]		The information of the parent node
 * @param newinode[OUT]	Point to the node information obtained by name
 * @param name[IN]		The name to look up
 *
 * @return error number
 *
 * */
extern  int hsi_nfs3_lookup(struct hsfs_inode *parent, struct hsfs_inode 
			**newinode, const char *name);

/**
 * hsi_nfs3_alloc_node: allocate memory for a node 
 *
 * @param sb[IN]:       the information of the superblock
 * @param pfh[IN]:      a pointer to the filehandle of node 
 * @param ino[IN]:      the inode number
 * @param pattr[IN]:    a pointer to the attributes of node 
 * @return:             a pointer to the object if successfull,else NULL
 *
 * */
extern struct hsfs_inode *hsi_nfs3_alloc_node(struct hsfs_super *sb, nfs_fh3 
					*pfh, uint64_t ino, fattr3 *pattr);

/**
 * hsi_nfs3_ifind:		Look up a node from hash table
 *
 * @param sb[IN]:		The information of the superblock
 * @param fh[IN]:		Filehandle of the parent directory
 * @param attr[IN]:		Node attributes

 *
 * @return:			A pointer to the node information
 *
 * */
extern struct hsfs_inode *hsi_nfs3_ifind(struct hsfs_super *sb, nfs_fh3 *fh, 
					 fattr3 *attr);

/**
 * The four internal functions used to convert fattr3 to struct stat, 
 * struct stat to hsfs_sattr by to_set and call nfs procedures are defined 
 * below, which use the standard errno as return valules defined in errno.h.
 */

extern int hsi_nfs3_getattr(struct hsfs_inode *inode);
extern int hsi_nfs3_fattr2stat(fattr3 *fattr, struct stat *st);
extern int hsi_nfs3_stat2sattr(struct stat *st, int to_set, 
			       struct hsfs_sattr *attr);
extern int hsi_nfs3_setattr(struct hsfs_inode *inode, struct hsfs_sattr *attr);

/**
 * hsi_nfs3_statfs
 * Get Dynamic file system information
 */
extern int hsi_nfs3_statfs (struct hsfs_inode *inode);

/**
 * hsi_super2statvfs
 * Change some fields of hsfs_super to statvfs
 */
extern int hsi_super2statvfs (struct hsfs_super *sp, struct statvfs *stbuf);

/**
 * Read dir
 *
 * @param hrc       the content of the fuse_add_direntry and fi->fh
 * @param dircount  the maximum number of bytes of directory information returned
 * @param maxcount  the maximum size of the READDIRPLUS3resok structure in bytes
 *
 * @return error number
 * */
extern int hsi_nfs3_readdir(struct hsfs_inode *hi, struct hsfs_readdir_ctx *hrc, 
						size_t maxcount);

/* Get NFS filesystem info */
extern int hsi_nfs3_fsinfo(struct hsfs_inode *inode);
extern int hsi_nfs3_pathconf(struct hsfs_inode *inode);

/**
 * hsi_nfs3_setxattr
 * set the extended attribute
*/
extern int hsi_nfs3_setxattr(struct hsfs_inode *hi, const char *value, int type,
				size_t size);
#endif
