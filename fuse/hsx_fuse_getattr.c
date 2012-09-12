#include <errno.h>
#include "hsi_nfs3.h"

void hsx_fuse_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
  	int err = 0;
	double to = 0;
	struct stat st;
	struct hsfs_inode *inode = NULL;
	struct hsfs_super *sb = NULL;
	
	DEBUG_IN("%s", "Enter hsx_fuse_getattr().\n");

	sb = (struct hsfs_super *) fuse_req_userdata(req);
	if (NULL == sb) {
		err = EINVAL;
		ERR("req is invalid.\n");
		goto out;
	}
	inode = hsx_fuse_iget(sb, ino);
	if (NULL == inode) {
		err = ENOENT;
		ERR("ino :%lu is invalid.\n", ino);
		goto out;
	}
	err = hsi_nfs3_getattr(inode);
	if(err)
		goto out;
	memset(&st, 0, sizeof(st));
	err = hsi_nfs3_fattr2stat(&inode->attr, &st);
 out:
	DEBUG_OUT("Leave hsx_fuse_getattr() with errno : %d.\n", err);
	if (err)
		fuse_reply_err(req, err);
	else {
		to = inode->attr.type == NF3DIR ? sb->acdirmin : sb->acregmin;
		fuse_reply_attr(req, &st, to);
	}
}
