#include "hsi_nfs3.h"
#include "hsfs.h"
/**
 *  hsx_fuse_fill_reply
 *  fill the struct fuse_entry_param with struct hsfs_inode.fattr3
 *  struct fuse_entry_param provide to fuse_reply_entry
 *  Edit:2012/09/03 Hu yuwei
 *  
 */
#define S_ISREG 1
#define S_ISDIR 2
void hsx_fuse_fill_reply (struct hsfs_inode *inode, struct fuse_entry_param *e)
{	
	DEBUG_IN(" in %s\n", "hsx_fuse_fill_reply");
	if(NULL == inode) {
		ERR("NULL POINTER in hsx_fuse_fill_reply");
		return;
	}
	if (0 == hsi_nfs3_fattr2stat(&(inode->attr), &(e->attr))) {
		e->ino = inode->ino;
		if (S_ISREG == inode->attr.type) {
			e->attr_timeout = inode->sb->acregmin;
			e->entry_timeout = inode->sb->acregmax;
		}
		else if (S_ISDIR == inode->attr.type) {
			e->attr_timeout = inode->sb->acdirmin;
			e->entry_timeout = inode->sb->acdirmax;
		}else
			ERR("Error in get file type!\n");
		return;
	}else
		ERR ("Error in fattr2stat\n");
	DEBUG_OUT("OUT %s\n", "hsx_fuse_fill_reply");
	return;
}
