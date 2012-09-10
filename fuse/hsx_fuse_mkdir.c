#include <fuse/fuse_lowlevel.h>

#include "hsfs.h"
/**
 *  hsx_fuse_mkdir
 *  function for make dir
 *  Edit:2012/09/05 Hu yuwei
 *  
 *  @param fuse_req_t:data about RPC fuse, the function will use struct CLIENT
 */
void hsx_fuse_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
	       	mode_t mode)
{
	int err = 0;
	struct hsfs_inode *hi_parent;
	struct hsfs_inode *new;
	//struct hsfs_super *sp;
	struct fuse_entry_param *e;
	char *dirname = name;
	hi_parent = (struct hsfs_inode *) malloc (sizeof(struct hsfs_inode));
	new = (struct hsfs_inode *) malloc (sizeof(struct hsfs_inode));
	hi_parent->sb = (struct hsfs_super *) malloc (sizeof(struct hsfs_super));
	e = (struct fuse_entry_param *) malloc (sizeof(struct fuse_entry_param));
	memset(e, 0, sizeof(struct fuse_entry_param));
	memset(hi_parent->sb, 0, sizeof(struct hsfs_super));
	memset(new, 0, sizeof(struct hsfs_inode));
	memset(hi_parent, 0, sizeof(struct hsfs_inode));

	hi_parent->sb = fuse_req_userdata(req);
	hi_parent = hsx_fuse_iget(hi_parent->sb, parent);
	err = hsi_nfs3_mkdir(hi_parent, &new, dirname, mode);
	if(0 != err )
	{
		fuse_reply_err(req, err);
		INFO("A %d",err);
		goto out;
	}
	else
	{
		hsx_fuse_fill_reply(new, e);
		fuse_reply_entry(req, e);
		goto out;
	}
out:
	INFO("I 1--! was here in hsx_fuse_mkdir.c");
	free(new);
	INFO("I 2--! was here in hsx_fuse_mkdir.c");
	free(e);
	INFO("I 4--! was here in hsx_fuse_mkdir.c");
	return;
};


