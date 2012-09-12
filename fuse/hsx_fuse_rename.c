/**
 * hsx_fuse_rename.c
 * yanhuan
 * 2012.9.5
 **/

#include "hsfs.h"
#include "hsi_nfs3.h"
#include "hsx_fuse.h"
#include <errno.h>

extern void hsx_fuse_rename(fuse_req_t req, fuse_ino_t parent, const char *name,
				fuse_ino_t newparent, const char *newname)
{
	DEBUG_IN(" %s to %s", name, newname);
	int err = 0;
	if (name == NULL) {
		fprintf(stderr, "%s source name is NULL\n", progname);
		err = ENOENT;
		goto out;
	}
	else if (newname == NULL) {
		fprintf(stderr, "%s target name is NULL\n", progname);
		err = ENOENT;
		goto out;
	}
	else if (!(strcmp(name, ""))) {
		fprintf(stderr, "%s source name is empty\n", progname);
		err = ENOENT;
		goto out;
	}
	else if (!(strcmp(newname, ""))) {
		fprintf(stderr, "%s target name is empty\n", progname);
		err = ENOENT;
		goto out;
	}
	else if (strlen(name) > NAME_MAX) {
		fprintf(stderr, "%s source name is too long\n", progname);
		err = ENAMETOOLONG;
		goto out;
	}
	else if (strlen(newname) > NAME_MAX) {
		fprintf(stderr, "%s target name is too long\n", progname);
		err = ENAMETOOLONG;
		goto out;
	}

	struct hsfs_super *sb = NULL;
	struct hsfs_inode *hi = NULL, *newhi = NULL;
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
	if (!(newhi = hsx_fuse_iget(sb, newparent))) {
		ERR("%s get hsfs inode failed\n", progname);
		err = EIO;
		goto out;
	}

	if ((err = hsi_nfs3_rename(hi, name, newhi, newname))) {
		goto out;
	}
out:
	fuse_reply_err(req, err);
	DEBUG_OUT(" %s to %s errno:%d", name, newname, err);
}

