#include "hsi_nfs3.h"
#include "hsfs.h"
#include "nfs3.h"

void debug_log_stat(struct stat *st _U_)
{
	DEBUG("STAT INO(%lu), U:%u, G:%u, S:%llu, B:%llu, M:0%o",
	      st->st_ino, st->st_uid, st->st_gid, st->st_size, st->st_blocks,
	      (unsigned int)st->st_mode);
	
}

void hsx_fuse_fill_reply (struct hsfs_inode *inode, struct fuse_entry_param *e)
{	
	hsfs_generic_fillattr(inode, &(e->attr));
	debug_log_stat(&(e->attr));

	e->ino = inode->ino;
	e->generation = 0;
	e->attr_timeout = inode->sb->acdirmin;
	e->entry_timeout = inode->sb->acregmin;
	
	DEBUG("FUSE Entry INO(%lu), Attr_Timeo(%f), Entry_Timeo(%f)",
	      e->ino, e->attr_timeout, e->entry_timeout);
}
