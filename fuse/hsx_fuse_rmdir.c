#include <fuse/fuse_lowlevel.h>

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
	DEBUG_IN(" in %s.\n","hsx_fuse_rmdir");
	int err = 0;
	struct hsfs_inode *hi_parent = NULL;
	struct hsfs_super *sb = NULL;
	const char *dirname =name;

	sb = fuse_req_userdata(req);
	hi_parent = hsx_fuse_iget(sb, parent);
	
	err = hsi_nfs3_rmdir(hi_parent, dirname);
	fuse_reply_err(req, err);
	DEBUG_OUT(" out, errno: %d, %s\n", err, "hsx_fuse_rmdir");
	return;
};
