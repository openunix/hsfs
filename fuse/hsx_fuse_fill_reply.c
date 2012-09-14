#include "hsi_nfs3.h"
#include "hsfs.h"
#include "nfs3.h"
/**
 *  hsx_fuse_fill_reply
 *  fill the struct fuse_entry_param with struct hsfs_inode.fattr3
 *  struct fuse_entry_param provide to fuse_reply_entry
 *  Edit:2012/09/03 Hu yuwei
 *  
 */
int hsx_fuse_fill_reply (struct hsfs_inode *inode, struct fuse_entry_param *e)
{	
	DEBUG_IN(" ino %d.\n", inode->ino);
	if(NULL == inode) {
		ERR("NULL POINTER in hsx_fuse_fill_reply");
		goto out;
	}
	if (0 == hsi_nfs3_fattr2stat(&(inode->attr), &(e->attr))) {
		e->ino = inode->ino;
		if (NF3REG == inode->attr.type) {
			e->attr_timeout = inode->sb->acregmin;
			e->entry_timeout = inode->sb->acregmax;
		}
		else if (NF3DIR == inode->attr.type) {
			e->attr_timeout = inode->sb->acdirmin;
			e->entry_timeout = inode->sb->acdirmax;
		}else {
			ERR("Error in get file type!\n");
			goto out;
		}
		DEBUG_OUT("OUT %s\n", "hsx_fuse_fill_reply");
		return 1;
	}else {
		ERR ("Error in fattr2stat\n");
	}

out:
	DEBUG_OUT("OUT ino %d.\n", inode->ino);
	return 0;
}
