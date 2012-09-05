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
	struct hsfs_super *sp;
	struct fuse_entry_param *e;
	char *dirname = name;
	
	hi_parent = (struct hsfs_inode *) malloc (sizeof(struct hsfs_inode));
	new = (struct hsfs_inode *) malloc (sizeof(struct hsfs_inode));
	sp = (struct hsfs_super *) malloc (sizeof(struct hsfs_super));
	e = (struct fuse_entry_param *) malloc (sizeof(struct fuse_entry_param));
	
	memset(e, 0, sizeof(struct fuse_entry_param));
	memset(sp, 0, sizeof(struct hsfs_super));
	memset(new, 0, sizeof(struct hsfs_inode));
	memset(hi_parent, 0, sizeof(struct hsfs_inode));

	sp = fuse_req_userdata(req);
	hi_parent = hsx_fuse_iget(&sp, req);
	err = hsi_nfs3_mkdir(hi_parent, &new, dirname, mode);
	
	if(0 != err )
	{
		fuse_reply_err(req, err);
		goto out;
	}
	else
	{
		hsx_fuse_fill_reply(new, &e);
		fuse_reply_entry(req, e);
		goto out;
	}
out:
	free(hi_parent);
	free(new);
	free(sp);
	free(e);
	return;
};


