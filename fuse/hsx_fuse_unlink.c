/**
 * hsx_fuse_unlink.c
 * yanhuan
 * 2012.9.5
 **/

#include "hsfs.h"
#include "hsi_nfs3.h"
#include "hsx_fuse.h"
#include <errno.h>

extern void hsx_fuse_unlink(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	DEBUG_IN(" %s", name);
	int err = 0;
	if (name == NULL) {
		ERR("%s name to remove is NULL\n", progname);
		err = ENOENT;
		goto out;
	}
	else if (!(strcmp(name, ""))) {
		ERR("%s name to remove is empty\n", progname);
		err = ENOENT;
		goto out;
	}
	else if (strlen(name) > NAME_MAX) {
		ERR("%s name to remove is too long\n", progname);
		err = ENAMETOOLONG;
		goto out;
	}

	struct hsfs_super *sb = NULL;
	struct hsfs_inode *hi = NULL;
	if (!(sb = fuse_req_userdata(req))) {
		ERR("%s get user data failed\n", progname);
		err = EIO;
		goto out;
	}
	if (!(hi = hsx_fuse_iget(sb, parent))) {
		ERR("%s get hsfs inode failed\n", progname);
		err = EIO;
		goto out;
	}

	if ((err = hsi_nfs3_unlink(hi, name))) {
		goto out;
	}
out:
	fuse_reply_err(req, err);
	DEBUG_OUT(" %s errno:%d", name, err);
}

