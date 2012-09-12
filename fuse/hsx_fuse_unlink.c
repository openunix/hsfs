/**
 * hsx_fuse_unlink.c
 * yanhuan
 * 2012.9.5
 **/

#include "hsfs.h"
#include "hsi_nfs3.h"
#include "hsx_fuse.h"
#include <errno.h>

void hsx_fuse_unlink(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	int err = 0;
	struct hsfs_super *sb = NULL;
	struct hsfs_inode *hi = NULL;

	DEBUG_IN(" %s", name);

	if (name == NULL) {
		ERR("Name to remove is NULL\n");
		err = EINVAL;
		goto out;
	}
	else if (!(strcmp(name, ""))) {
		ERR("Name to remove is empty\n");
		err = EINVAL;
		goto out;
	}
	else if (strlen(name) > NAME_MAX) {
		ERR("Name to remove is too long\n");
		err = ENAMETOOLONG;
		goto out;
	}

	if (!(sb = fuse_req_userdata(req))) {
		ERR("Get user data failed\n");
		err = EIO;
		goto out;
	}
	if (!(hi = hsx_fuse_iget(sb, parent))) {
		ERR("get hsfs inode failed\n");
		err = ENOENT;
		goto out;
	}

	if ((err = hsi_nfs3_unlink(hi, name))) {
		goto out;
	}
out:
	fuse_reply_err(req, err);
	DEBUG_OUT(" %s errno:%d", name, err);
}

