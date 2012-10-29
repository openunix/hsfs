#include "hsfs.h"
#include "hsx_fuse.h"
#include "hsi_nfs3.h"

void hsx_fuse_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	int err = 0;
	struct hsfs_inode *hi_parent = NULL;
	struct hsfs_super *sb = NULL;
	const char *dirname =name;
	DEBUG_IN(" name is %lu.\n", parent);

	if((sb = fuse_req_userdata(req)) == NULL) {
		ERR("ERR in fuse_req_userdata pointer sb is null");
		return;
	}
	if((hi_parent = hsfs_ilookup(sb, parent)) == NULL) {
		ERR("ERR in hsfs_ilookup pointer hi_parent is null");
		return;
	}
	
	err = hsi_nfs3_rmdir(hi_parent, dirname);
	fuse_reply_err(req, err);
	DEBUG_OUT(" out, errno: %d.", err);
	return;
};
