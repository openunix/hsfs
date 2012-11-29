
#include "hsfs.h"
#include "hsx_fuse.h"
#include "hsi_nfs3.h"

void hsx_fuse_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
	       	mode_t mode)
{	
	int err = 0;
	struct hsfs_inode *hi_parent = NULL;
	struct hsfs_inode *new = NULL;
	struct fuse_entry_param e;
	struct hsfs_super *sb = NULL;
	const char *dirname = name;
	unsigned long ref;

	DEBUG_IN("ino:%lu.\n", parent);

	memset(&e, 0, sizeof(struct fuse_entry_param));

	sb = fuse_req_userdata(req);
	hi_parent = hsfs_ilookup(sb, parent);
	
	if(NULL == sb) {
		ERR("ERR in fuse_req_userdata");
		goto out;
	}
	if(NULL == hi_parent) {
		ERR("ERR in hsfs_ilookup");
		goto out;
	}

	
	err = hsi_nfs3_mkdir(hi_parent, &new, dirname, mode);
	if(0 != err ) {
		fuse_reply_err(req, err);
		goto out;
	}

	if(new == NULL) {
		err = ENOMEM;
		fuse_reply_err(req, err);
		goto out;
	}

	ref = hsx_fuse_ref_xchg(new, 1);
	FUSE_ASSERT(ref == 0);	/* Should be a new one... */
	
	hsx_fuse_fill_reply(new, &e);
	fuse_reply_entry(req, &e);
out:
	DEBUG_OUT(" out errno is: %d\n", err);
	return;
};


