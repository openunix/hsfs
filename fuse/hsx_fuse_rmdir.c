
#include "hsfs.h"
#include "hsx_fuse.h"
#include "hsi_nfs3.h"
/**
 *  hsx_fuse_rmdir
 *  function for remove dir
 *  Edit:2012/09/05 Hu yuwei
 *  
 *  @param fuse_req_t:data of RPC&fuse 
 *
 */
void hsx_fuse_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	int err = 0;
	struct hsfs_inode *hi_parent = NULL;
	struct hsfs_super *sb = NULL;
	const char *dirname =name;
	DEBUG_IN(" in %s.\n","hsx_fuse_rmdir");

	if((sb = fuse_req_userdata(req)) == NULL) {
		ERR("ERR in fuse_req_userdata pointer sb is null");
		return;
	}
	if((hi_parent = hsx_fuse_iget(sb, parent)) == NULL) {
		ERR("ERR in hsx_fuse_iget pointer hi_parent is null");
		return;
	}
	
	err = hsi_nfs3_rmdir(hi_parent, dirname);
	fuse_reply_err(req, err);
	DEBUG_OUT(" out, errno: %d, %s\n", err, "hsx_fuse_rmdir");
	return;
};
