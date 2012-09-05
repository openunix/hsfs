/**
 * hsx_fuse_rename.c
 * yanhuan
 * 2012.9.5
 **/

#define MAXNAMELEN 255

extern void hsx_fuse_rename(fuse_req_t req, fuse_ino_t parent, const char *name,
				fuse_ino_t newparent, const char *newname)
{
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
	else if (strlen(name) > MAXNAMELEN) {
		fprintf(stderr, "%s source name is too long\n", progname);
		err = ENAMETOOLONG;
		goto out;
	}
	else if (strlen(newname) > MAXNAMELEN) {
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
	if (!(newhi = hsx_fuse_iget(sb, parent))) {
		ERR("%s get hsfs inode failed\n", progname);
		err = EIO;
		goto out;
	}

	if ((err = hsi_nfs3_rename(hi, name, newhi, newname))) {
		goto out;
	}
out:
	/* update ino <=> fh? */
	fuse_reply_err(req, err);
}

