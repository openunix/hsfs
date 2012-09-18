
#ifndef __HSX_FUSE_H__
#define __HSX_FUSE_H__ 

#include <sys/stat.h>

extern void hsx_fuse_init(void *data, struct fuse_conn_info *conn);

/**
 * @brief Make a directory
 *
 * Valid replies:
 *   fuse_reply_err
 *   fuse_reply_entry
 *
 * @param req[in] request handle
 * @param parent[in] inode number of the parent directory
 * @param name[in] to make
 * @param mode[in] the mode to make
 **/
extern void hsx_fuse_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
	       	mode_t mode);

/**
 * @brief Remove a directory
 *
 * Valid replies:
 *   fuse_reply_err
 *
 * @param req[in] request handle
 * @param parent[in] inode number of the parent directory
 * @param name[in] to remove
 **/
extern void hsx_fuse_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name);

/**
 * @brief Fill the param in the entry
 *
 * @param inode[in] data fo hsfs_inode
 * @param e[in] the param of the entry
 *
 * @param err[out] the error number of the function
 **/
extern int hsx_fuse_fill_reply(struct hsfs_inode *inode,
	       	struct fuse_entry_param *e);
			
/**
 * @brief Read data
 *
 * Valid replies:
 *   fuse_reply_buf
 *   fuse_reply_err
 *
 * @param req[in] request handle
 * @param ino[in] the inode number
 * @param size[in] number of bytes to read
 * @param off[in] offset to read from
 * @param fi[in] file information
 **/
extern void hsx_fuse_read(fuse_req_t req, fuse_ino_t ino, size_t size,
				off_t off, struct fuse_file_info *fi);
				
/**
 * @brief Write data
 *
 * Valid replies:
 *   fuse_reply_write
 *   fuse_reply_err
 *
 * @param req[in] request handle
 * @param ino[in] the inode number
 * @param buf[in] data to write
 * @param size[in] number of bytes to write
 * @param off[in] offset to read from
 * @param fi[in] file information
 **/
extern void hsx_fuse_write(fuse_req_t req, fuse_ino_t ino, const char *buf,
				size_t size, off_t off, struct fuse_file_info *fi);

/**
 * @brief Remove a file
 *
 * Valid replies:
 *   fuse_reply_err
 *
 * @param req[in] request handle
 * @param parent[in] inode number of the parent directory
 * @param name[in] to remove
 **/
extern void hsx_fuse_unlink(fuse_req_t req, fuse_ino_t parent, const char *name);

/** 
 * @brief Rename a file
 *
 * Valid replies:
 *   fuse_reply_err
 *
 * @param req[in] request handle
 * @param parent[in] inode number of the old parent directory
 * @param name[in] old name
 * @param newparent[in] inode number of the new parent directory
 * @param newname[in] new name
 **/
extern void hsx_fuse_rename(fuse_req_t req, fuse_ino_t parent, const char *name, 
			    fuse_ino_t newparent, const char *newname);

/**
 * @brief Read the content of the symlink
 *
 * Valid replies:
 * fuse_reply_readlink
 * fuse_reply_err
 *
 * @param req[in]	 request handle
 * @param ino[in]	 the inode number
 **/
extern void hsx_fuse_readlink(fuse_req_t req, fuse_ino_t ino);

/**
 * @brief Create a symbolic link
 *
 * Valid replies:
 * fuse_reply_entry
 * fuse_reply_err
 *
 * @param req[in]	 request handle
 * @param link[in]	 the contents of the symbolic link
 * @param parent[in]	 inode number of the parent directory
 * @param name[in]	 to create
 **/
extern void hsx_fuse_symlink(fuse_req_t req, const char *link, 
				fuse_ino_t parent, const char *name);


/**
 *@brief Request permission to an operation
 *
 * Valid replies:
 *   fuse_reply_err
 *
 * @param req[in] request handle
 * @param ino[in] inode number to request
 * @param mask[in] request permission 
 **/
extern void hsx_fuse_access(fuse_req_t req, fuse_ino_t ino, int mask);


/**
 * @brief Create a regular file.
 *
 * Valid replies:
 *   fuse_repay_create
 *   fuse_reply_err
 *
 * @param req[in] request handle
 * @param parent[in] the inode number of the parent directory
 * @param name[in] new file name
 * @param mode[in] access mode to the new file
 * @param fi[in] file information
 **/
extern void hsx_fuse_create(fuse_req_t req, fuse_ino_t parent, const char *name,
			    mode_t mode, struct fuse_file_info *fi);                              

/**
 * @brief Create a special file 
 *
 * Valid replies:
 *  fuse_reply_entry
 *  fuse_reply_err
 *
 * @param  req[in] request handle
 * @param  parent[in] the inode number of the parent directory 
 * @param  name[in] the name of the special file you want to create
 * @param  mode[in] file type and access mode of the file
 * @param  rdev[in] device number of the special file
 */
extern	void hsx_fuse_mknod(fuse_req_t	req, fuse_ino_t	parent,const char *name,
			 			mode_t mode, dev_t rdev);


/**
 * @brief Create a hard link to a file
 *
 * Valid replies:
 *  fuse_reply_entry
 *  fuse_reply_err
 *
 * @param  req[in] request handle
 * @param  ino[in] the old inode number
 * @param  newparent[in] inode number of the new parent directory
 * @param  newname[in] new name to create
 */
extern	void hsx_fuse_link(fuse_req_t req, fuse_ino_t ino, fuse_ino_t newparent,
				 const char *newname);

/**
 * @brief Look up a directory entry by name
 *
 * Valid replies:
 *  fuse_reply_entry
 *  fuse_reply_err
 *
 * @param  req[in] request handle
 * @param  ino[in] inode number of the parent directory 
 * @param  name[in] the name to look up
 */
extern  void  hsx_fuse_lookup(fuse_req_t req, fuse_ino_t ino, const char *name);


/**
 * @brief Forget about an inode
 *
 * Valid replies:
 *  fuse_reply_none
 *
 * @param  req[in] request handle
 * @param  ino[in] the inode number to forget 
 * @param  nlookup[in] the number of lookups to forget  
 */
extern  void  hsx_fuse_forget(fuse_req_t req, fuse_ino_t ino, unsigned long nlookup);

/**
 * @Open a file
 * 
 * @Valid replies:
 *  fuse_reply_err 
 *  fuse_reply_open
 *
 * @param req[in] request handle
 * @param ino[in] the old inode number
 * @param fi[in]  file information
 * */
extern void hsx_fuse_open (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
/**
 * @Release a file
 *
 * @Valid replies:
 *  fuse_reply_err
 *
 * @param req[in] request handle
 * @param ino[in] the old inode number
 * @param fi[in]  file information
 **/
extern void hsx_fuse_release (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);

/**
 * @brief Get file attributes
 * 
 * Valid replies:
       fuse_reply_err, fuse_reply_attr
 * @param req[in] request handle
 * @param ino[in] the inode number
 * @param fi[in] for future use, currently always NULL
 */
extern void hsx_fuse_getattr(fuse_req_t req, fuse_ino_t ino, 
			     struct fuse_file_info *fi);
/**
 * @breif Set file attributes
 *
 * Valid replies:
 *     fuse_reply_err, fuse_reply_attr
 * 
 * @param req[in] request handle
 * @param ino[in] the inode number
 * @param attr[in] the attributes
 * @param to_set[in] bit mask of attributes which should be set
 * @param fi[in] file information, or NULL
 */
extern void hsx_fuse_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
			     int to_set, struct fuse_file_info *fi);
/**
 * @Get the filesystem statistics
 *
 * Valid replies:
 *  fuse_reply_statfs fuse_reply_err
 *
 * @param req[in] request handle
 * @param ino[in] the inode number
 */
extern void hsx_fuse_statfs(fuse_req_t req, fuse_ino_t ino);

/**
 * @brief Read the content of the directory
 *
 * Valid replies:
 *   fuse_reply_buf
 *   fuse_reply_err
 *
 * @param req[in] request handle
 * @param ino[in] the inode number
 * @param size[in] the maximum number of bytes to send
 * @param off[in] the offset to continue reading the directory
 * @param fi[in] the file information
 **/
extern void hsx_fuse_readdir(fuse_req_t req,  fuse_ino_t ino,  size_t size,  off_t off,  struct fuse_file_info  *fi);

/**
 * @brief Open a directory
 * 
 * Valid replies:
 *   fuse_reply_open
 *   fuse_reply_err
 *
 * @param req[in] request handle
 * @param ino[in] the inode number
 * @param fi[in,out] the file information
 **/
extern void hsx_fuse_opendir(fuse_req_t req,  fuse_ino_t ino,  struct fuse_file_info  *fi);

/**
 * @Mount NFS filesystem (get root filehandle)
 *
 * @spec[in]	mount spec at remote node
 * @point[in]	mount point at local node
 * @args[out]	save fuse arguments which is mapping from nfs
 * @udata[out]	nfs arguments which is used for mtab
 * @userdata[out]	hsfs super block
 * 
 * @return fuse channel
 */
extern struct fuse_chan *hsx_fuse_mount(const char *spec, const char *point,
					struct fuse_args *args, char *udata,
					struct hsfs_super *userdata);

/**
 * @Unmount NFS filesystem (release root filehandle)
 *
 * @spec[in]	mount spec at remote node
 * @point[in]	mount point at local node
 * @ch[in]	fuse channel for hsfs
 * @userdata[out]	hsfs super block
 * 
 * @return errno number as Linux system
 */
extern int hsx_fuse_unmount(const char *spec, const char *point,
					struct fuse_chan *ch,
					struct hsfs_super *super);
extern void hsx_fuse_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
				 size_t size);

/**
 * hsx_fuse_setxattr
 * set the extended attribute
 **/
extern void hsx_fuse_setxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
				const char *value, size_t size, int flags );

#endif
