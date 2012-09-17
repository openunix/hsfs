
#ifndef __HSX_FUSE_H__
#define __HSX_FUSE_H__ 

#include <sys/stat.h>

extern void hsx_fuse_init(void *data, struct fuse_conn_info *conn);

extern void hsx_fuse_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
	       	mode_t mode);

extern void hsx_fuse_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name);

extern void hsx_fuse_fill_reply(struct hsfs_inode *inode,
	       	struct fuse_entry_param *e);

extern void hsx_fuse_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi);

extern void hsx_fuse_write(fuse_req_t req, fuse_ino_t ino, const char *buf, size_t size, off_t off, struct fuse_file_info *fi);

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
 * read the content of the symlink
 * Valid replies:
 *   fuse_reply_readlink
 *   fuse_reply_err
 *
 * @param req request handle
 * @param ino the inode number
 **/
extern void hsx_fuse_readlink(fuse_req_t req, fuse_ino_t ino);

/**
 * Create a symbolic link
 * Valid replies:
 *   fuse_reply_entry
 *   fuse_reply_err
 *
 * @param req request handle
 * @param link the contents of the symbolic link
 * @param parent inode number of the parent directory
 * @param name to create
 **/
extern void hsx_fuse_symlink(fuse_req_t req, const char *link,                      
			     fuse_ino_t parent, const char *name);


/**
 * hsx_fuse_access
 *
 * To request whether or not you have the permission to perform an operation.
 *
 * Valid replies:
 *   fuse_reply_err
 *
 * @param req request handle
 * @param ino the inode number where you want to perform an operation
 * @param mask the permission you want to request
 **/
extern void hsx_fuse_access(fuse_req_t req, fuse_ino_t ino, int mask);


/**
 * hsx_fuse_create
 *
 * To create a new regular file.
 *
 * Valid replies:
 *		fuse_repay_create
 *   fuse_reply_err
 *
 * @param req request handle
 * @param parent the inode number of the parent directory where 
 *								you want to create a new regular file
 * @param name the name of the file you want to create
 * @param mode the access mode of the file
 * @param fi	the fuse_file_info which identify the way to create the new file
 **/
extern void hsx_fuse_create(fuse_req_t req, fuse_ino_t parent, const char *name,
			    mode_t mode, struct fuse_file_info *fi);                              

/**
 * hsx_fuse_mknod
 *
 * To create special file : CHR, BLK,FIFO,SOCK, err number will return if the type is not one of them
 *
 * Valid replies:
 * 		fuse_reply_entry
 * 		fuse_reply_err
 *
 * @param  req		request handle
 * @param  parent	the inode number of the parent directory where you want to create the special file
 * @param  name		the name of the special file you want to create
 * @param  mode		file type and access mode of the file
 * @param  rdev		device number of the special file: only used for CHR and BLK
 * **/
extern	void hsx_fuse_mknod(fuse_req_t	req, fuse_ino_t	parent,const char *name,
			 			mode_t mode, dev_t rdev);


/**
 * hsx_fuse_link
 * 
 * To create a hard link to a file
 *
 * Valid replies:
 * 		fuse_reply_entry
 * 		fuse_reply_err
 *
 * @param  req		request handle
 * @param  ino		the old inode number
 * @param  newparent	inode number of the new parent directory
 * @param  newname	new name to create
 * */
extern	void hsx_fuse_link(fuse_req_t req, fuse_ino_t ino, fuse_ino_t newparent,
				 const char *newname);

/*hsx_fuse_lookup  and  hsx_fuse_forget declaration */

extern  void  hsx_fuse_lookup(fuse_req_t req, fuse_ino_t ino, const char *name);

extern  void  hsx_fuse_forget(fuse_req_t req, fuse_ino_t ino, unsigned long nlookup);

/**
 * hsx_fuse_open & hsx_fuse_release
 * */
extern void hsx_fuse_open (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);

extern void hsx_fuse_release (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);

/**
 * The two functions used to get and set file attributions are defined below, 
 * which use the fues_reply_err or fues_reply_attr to return values, showed 
 * as follows.
 *
 * return functions:
 * int fuse_reply_err(fuse_req_t req, int err);
 * int fuse_reply_attr(fuse_req_t req, const struct stat *attr, 
 * double attr_timeout);
 */
extern void hsx_fuse_getattr(fuse_req_t req, fuse_ino_t ino, 
			     struct fuse_file_info *fi);
extern void hsx_fuse_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
			     int to_set, struct fuse_file_info *fi);
/**
 * hsx_fuse_statfs
 * Get the filesystem statistics
 * reply:
 * fuse_reply_statfs
 * fuse_reply_err
 **/
extern void hsx_fuse_statfs(fuse_req_t req, fuse_ino_t ino);

/**
 * read the content of the directory
 * Valid replies:
 *   fuse_reply_buf
 *   fuse_reply_err
 *
 * @param req         request handle
 * @param ino         the inode number
 * @param size        the maximum number of bytes to send
 * @param off         the offset to continue reading the directory
 * @param fi          the file information
 **/
extern void hsx_fuse_readdir(fuse_req_t req,  fuse_ino_t ino,  size_t size,  off_t off,  struct fuse_file_info  *fi);

/**
 * open a directory
 * Valid replies:
 *   fuse_reply_open
 *   fuse_reply_err
 *
 * @param req         request handle
 * @param ino         the inode number
 * @param fi          the file information
 **/
extern void hsx_fuse_opendir(fuse_req_t req,  fuse_ino_t ino,  struct fuse_file_info  *fi);

extern struct fuse_chan *hsx_fuse_mount(const char *spec, const char *point,
					struct fuse_args *args, char *udata,
					struct hsfs_super *userdata);
extern int hsx_fuse_unmount(const char *spec, const char *point,
					struct fuse_chan *ch,
					struct hsfs_super *super);

#endif
