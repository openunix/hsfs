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
 * @brief Read file 
 *
 * @param rw[in,out] the content of the read operation 
 *
 * @return error number
 **/
extern int hsi_nfs3_read(struct hsfs_rw_info* rw);


/**  
 * @brief Write file
 *
 * @param rw[int,out] the content of the write operation 
 *
 * @return error number
 **/
extern int hsi_nfs3_write(struct hsfs_rw_info* rw);


/**
 * @brief Request permission to an operation.
 *
 * @param hi[in] the information of the inode requested
 * @param mask[in] the permission requested
 *
 * return error number
 **/
extern int hsi_nfs3_access(struct hsfs_inode *hi, int mask);


/**
 * @brief Create a regular file.
 *
 * @param hi[in] the information of the parent directory
 * @param newhi[out] the information of the file created
 * @param name[in] the name of the file you want to create
 * @param mode[in] the access mode of the file
 *
 * @return error number
 */
extern int hsi_nfs3_create(struct hsfs_inode *hi, struct hsfs_inode **newhi,
			   const char *name, mode_t mode);


/**
 *  @brief Create a special file
 *
 *  @param parent[in] the inode of the parent directory
 *  @param newinode[out] the inode of the created file
 *  @param name[in] the name to create
 *  @param mode[in] file type and access mode of the file
 *  @param rdev[in] device number of the file: only used for CHR and BLK
 *  
 *  @return error number
 */
extern	int hsi_nfs3_mknod(struct hsfs_inode *parent, struct hsfs_inode 
		  	**newinode, const char *name,mode_t mode, dev_t rdev);

/**
 * @brief Create a hard link to a file
 *
 * @param ino[in] the inode of the file
 * @param newparent[in] the inode of the new parent
 * @param newinode[out] the inode of the linked file
 * @param name[in] the name to of the link to create
 * 
 * @return error number
 */
extern 	int hsi_nfs3_link(struct hsfs_inode *inode, struct hsfs_inode *newparent, 
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
 * @brief Get file attributes
 *
 * @param inode[in,out] the info of file containing the attributes as return 
 * 
 * @return error number
 */
extern int hsi_nfs3_getattr(struct hsfs_inode *inode);

/**
 * @brief Convert struct fattr3 to struct stat
 *                                                                                                                                         
 * @param fattr[in] the attributes 
 * @param st[out] the result of the convertion
 * 
 * @return error number
 */
extern int hsi_nfs3_fattr2stat(struct fattr3 *fattr, struct stat *st);

/**
 * @brief Convert struct stat to struct hsfs_sattr
 *
 * @param st[in] the struct stat of file attributes
 * @param to_set[in] bit mask of attributes which should be set
 * @param attr[out] the result of the convertion
 *
 * @return error number
 */
extern int hsi_nfs3_stat2sattr(struct stat *st, int to_set, 
			       struct hsfs_sattr *attr);
/**
 * @brief Set file attributes
 * 
 * @param inode[in,out] the info of file containing the attributes as return
 * @param attr[in] the attributes
 *
 * @return error number
 */
extern int hsi_nfs3_setattr(struct hsfs_inode *inode, struct hsfs_sattr *attr);

/**
 * @Get Dynamic file system information
 *
 * @param inode[in,out] the information of inode
 * @return error number
 */
extern int hsi_nfs3_statfs (struct hsfs_inode *inode);

/**
 * hsi_super2statvfs
 * Change some fields of hsfs_super to statvfs
 */
extern int hsi_super2statvfs (struct hsfs_super *sp, struct statvfs *stbuf);

/**
 * @brief Read dir
 *
 * @param parent[in] get the file handle
 * @param hrc[in,out] the content of the fuse_add_direntry and fi->fh
 * @param maxcount[out] the maximum size of the server return in bytes
 *
 * @return error number
 * */
extern int hsi_nfs3_readdir(struct hsfs_inode *parent, struct hsfs_readdir_ctx 
						*hrc, size_t maxcount);

/**
 * @Get static file system information of NFS
 *
 * @inode: hsfs inode of root directory
 *
 * @return errno number as Linux system
 */
extern int hsi_nfs3_fsinfo(struct hsfs_inode *inode);

/**
 * @Retrieve POSIX information of NFS
 *
 * @inode: hsfs inode of root directory
 *
 * @return errno number as Linux system
 */
extern int hsi_nfs3_pathconf(struct hsfs_inode *inode);

/**
 * @Potting clnt_call
 *
 * @sb: super block of hsfs
 * @procnum: NFS procedure number(macro defined in nfs3.h)
 * @inproc: function which is used to encode the procedure's parameters
 * @in: the address of the procedure's argument(s)
 * @outproc: function which is used to decode the procedure's results
 * @out:	the address of where to place the result(s)
 *
 * @return errno number as Linux system
 */
extern int hsi_nfs3_clnt_call(struct hsfs_super *sb, unsigned long procnum,
				xdrproc_t inproc, char *in,
				xdrproc_t outproc, char *out);

/**
 * hsi_nfs3_setxattr
 * set the extended attribute
extern int hsi_nfs3_setxattr(struct hsfs_inode *hi, const char *value, int type,
				size_t size);*/
#endif
