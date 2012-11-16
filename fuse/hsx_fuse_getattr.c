#include <errno.h>
#include "hsi_nfs3.h"

void hsx_fuse_getattr(fuse_req_t req, fuse_ino_t ino,
		      struct fuse_file_info *fi)
{
  	int err = 0;
	double to = 0;
	struct stat st;
	struct hsfs_inode *inode = NULL;
	struct hsfs_super *sb = NULL;
	
	DEBUG_IN("(%p, %lu, %p)", req, ino, fi);

	sb = (struct hsfs_super *) fuse_req_userdata(req);
	if (NULL == sb) {
		err = EINVAL;
		ERR("req is invalid.\n");
		goto out;
	}
	inode = hsfs_ilookup(sb, ino);
	if (NULL == inode) {
		err = ENOENT;
		ERR("ino :%lu is invalid.\n", ino);
		goto out;
	}
	memset(&st, 0, sizeof(st));
	err = hsi_nfs3_getattr(inode, &st);
	if(err)
		goto out;
 out:
	DEBUG_OUT("ino : %lu ,with errno : %d.\n", inode->ino,  err);
	if (err)
		fuse_reply_err(req, err);
	else {
		to = S_ISDIR(inode->i_mode) ? sb->acdirmin : sb->acregmin;
		fuse_reply_attr(req, &st, to);
	}
}
