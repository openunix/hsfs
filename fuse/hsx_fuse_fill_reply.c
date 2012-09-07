#include <fuse/fuse_lowlevel.h>

#include "hsfs.h"
/**
 *  hsx_fuse_fill_reply
 *  fill the struct fuse_entry_param with struct hsfs_inode.fattr3
 *  struct fuse_entry_param provide to fuse_reply_entry
 *  Edit:2012/09/03 Hu yuwei
 *  
 */
void hsx_fuse_fill_reply (struct hsfs_inode *inode, struct fuse_entry_param *e)
{	
	if (0 == hsi_nfs3_fattr2stat(&(inode->attr), &(e->attr))) {
		e->ino = inode->ino;
		if (1 == inode->attr.type) {
			e->attr_timeout = inode->sb->acregmin;
			e->entry_timeout = inode->sb->acregmax;
		}
		else if (2 == inode->attr.type) {
			e->attr_timeout = inode->sb->acdirmin;
			e->entry_timeout = inode->sb->acdirmax;
		}
		else
			ERR("Error in get file type!\n");
		return;
	}
		
	else
		printf ("Error in fattr2stat\n");
	return;
};


