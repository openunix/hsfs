/**
 * hsx_fuse_statfs
 * liuyoujin
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/errno.h>
#include "hsi_nfs3.h"
#include "hsx_fuse.h"
#include "log.h"
extern void hsx_fuse_statfs(fuse_req_t req, fuse_ino_t ino)
{
DEBUG_IN("%s","...");
	struct statvfs stbuf;
        struct hsfs_super *super = fuse_req_userdata(req);
	struct hsfs_inode *root = super->root;
	struct hsfs_super *sp = root->sb;
	int err=0;
	err = hsi_nfs3_statfs(root);
        if (err){
		fuse_reply_err (req, err);
		goto out;
	}
	err = hsi_super2statvfs(sp, &stbuf);
	if (err){
		err = EINVAL;
		fuse_reply_err (req, err);
		goto out;
	}
	fuse_reply_statfs (req, &stbuf);
out:
DEBUG_OUT ("%s","...");
	return;

}

