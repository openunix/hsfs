/**
 * hsx_fuse_rename.c
 * yanhuan
 * 2012.9.5
 **/

#include "hsfs.h"
#include "hsi_nfs3.h"
#include "hsx_fuse.h"
#include <errno.h>

void hsx_fuse_rename(fuse_req_t req, fuse_ino_t parent, const char *name,
				fuse_ino_t newparent, const char *newname)
{
	int err = 0;
	struct hsfs_super *sb = NULL;
	struct hsfs_inode *hi = NULL, *newhi = NULL;

	DEBUG_IN(" %s to %s", name, newname);

	if (name == NULL) {
		ERR("Source name is NULL\n");
		err = EINVAL;
		goto out;
	}
	else if (newname == NULL) {
		ERR("Target name is NULL\n");
		err = EINVAL;
		goto out;
	}
	else if (!(strcmp(name, ""))) {
		ERR("Source name is empty\n");
		err = EINVAL;
		goto out;
	}
	else if (!(strcmp(newname, ""))) {
		ERR("Target name is empty\n");
		err = EINVAL;
		goto out;
	}
	else if (strlen(name) > NAME_MAX) {
		ERR("Source name is too long\n");
		err = ENAMETOOLONG;
		goto out;
	}
	else if (strlen(newname) > NAME_MAX) {
		ERR("Target name is too long\n");
		err = ENAMETOOLONG;
		goto out;
	}

	if (!(sb = fuse_req_userdata(req))) {
		ERR("Get user data failed\n");
		err = EIO;
		goto out;
	}
	if (!(hi = hsx_fuse_iget(sb, parent))) {
		ERR("Get hsfs inode failed\n");
		err = ENOENT;
		goto out;
	}
	if (!(newhi = hsx_fuse_iget(sb, newparent))) {
		ERR("Get hsfs inode failed\n");
		err = ENOENT;
		goto out;
	}

	if ((err = hsi_nfs3_rename(hi, name, newhi, newname))) {
		goto out;
	}
out:
	fuse_reply_err(req, err);
	DEBUG_OUT(" %s to %s errno:%d", name, newname, err);
}

