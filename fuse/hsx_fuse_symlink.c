/*
 *void hsx_fuse_symlink(fuse_req_t req, const char *link,
 * 			fuse_ino_t parent, const char *name)
 *xuehaitao
 *2012.9.6
 */

#include "hsi_nfs3.h"
#include "hsx_fuse.h"
#include "log.h"
#include <errno.h>

void hsx_fuse_symlink(fuse_req_t req, const char *link,
			fuse_ino_t parent, const char *name)
{
	unsigned long ref = 0;
	int err = 0;
	struct hsfs_super *sb_parent = NULL;
	struct hsfs_inode *nfs_parent = NULL;
	struct hsfs_inode *new = NULL;
	struct fuse_entry_param e;

	DEBUG_IN("%s\n","hsx_fuse_symlink.");

	sb_parent = fuse_req_userdata(req);
	if(!sb_parent){
		ERR("%s gets inode->sb fails \n", progname);
		err = ENOENT;
		goto out;
	}

	nfs_parent = hsfs_ilookup(sb_parent, parent);
	if(!nfs_parent){
		ERR("%s gets inode fails \n", progname);
		err = ENOENT;
		goto out;
	}

	err = hsi_nfs3_symlink(nfs_parent, &new, link, name);
	if(err != 0)
		goto out;

	if(new == NULL) {
		err = ENOMEM;
		goto out;
	}

	ref = hsx_fuse_ref_xchg(new, 1);
	FUSE_ASSERT(ref == 0);	/* Should be a new one... */

	hsx_fuse_fill_reply(new, &e);
	fuse_reply_entry(req, &e);
out:
	if(err != 0){
		fuse_reply_err(req, err);
	}
	DEBUG("with errno %d\n", err);
	return;
}
