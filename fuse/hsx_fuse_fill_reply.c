#include "hsi_nfs3.h"
#include "hsfs.h"
#include "nfs3.h"

void hsx_fuse_fill_reply (struct hsfs_inode *inode, struct fuse_entry_param *e)
{	
	DEBUG_IN(" ino %lu.\n", inode->ino);

	hsfs_generic_fillattr(inode, &(e->attr));
	e->ino = inode->ino;
	e->attr_timeout = inode->sb->acdirmin;
	e->entry_timeout = inode->sb->acregmin;
	
	DEBUG_OUT("OUT %s\n", "hsx_fuse_fill_reply");
}
