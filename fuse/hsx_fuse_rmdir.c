#include <fuse/fuse_lowlevel.h>

#include "hsfs.h"
/**
 *  hsx_fuse_rmdir
 *  function for remove dir
 *  Edit:2012/09/05 Hu yuwei
 *  
 *  @param fuse_req_t:data of RPC&fuse 
 *
 */
void hsx_fuse_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	int err = 0;
	struct hsfs_inode *hi_parent;
	struct hsfs_super *sp;
	char *dirname =name;

	hi_parent = (struct hsfs_inode *) malloc (sizeof(struct hsfs_inode));
	sp = (struct hsfs_super) malloc (sizeof(struct hsfs_super));

	memset(hi_parent, 0, sizeof(struct hsfs_inode));
	memset(sp, 0, sizeof(struct hsfs_super));

	sp = fuse_req_userdata(req);
	hi_parent = hsx_fuse_iget(&sp, req);
	
	err = hsi_nfs3_rmdir(hi_parent, dirname);

	fuse_reply_err(req, err);
	
	free(hi_parent);
	free(sp);

	return;
};
