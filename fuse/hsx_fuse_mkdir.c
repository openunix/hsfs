#include <fuse/fuse_lowlevel.h>

#include "hsfs.h"
#include "hsx_fuse.h"
#include "hsi_nfs3.h"
/**
 *  hsx_fuse_mkdir
 *  function for make dir
 *  Edit:2012/09/05 Hu yuwei
 *  
 *  @param fuse_req_t:data about RPC fuse, the function will use struct CLIENT
 */
void hsx_fuse_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
	       	mode_t mode)
{	DEBUG_IN(" in %s.\n","hsx_fuse_mkdir");
	int err = 0;
	struct hsfs_inode *hi_parent = NULL;
	struct hsfs_inode *new = NULL;
	struct fuse_entry_param *e = NULL;
	struct hsfs_super *sb = NULL;
	const char *dirname = name;
	
	e = (struct fuse_entry_param *) malloc (sizeof(struct fuse_entry_param));
	
	memset(e, 0, sizeof(struct fuse_entry_param));

	sb = fuse_req_userdata(req);
	hi_parent = hsx_fuse_iget(sb, parent);
	err = hsi_nfs3_mkdir(hi_parent, &new, dirname, mode);
	if(0 != err )
	{
		fuse_reply_err(req, err);
		goto out;
	}
	else
	{
		hsx_fuse_fill_reply(new, e);
		fuse_reply_entry(req, e);
		goto out;
	}
out:
	free(e);
	DEBUG_OUT(" out errno is: %d, %s\n", err, "hsx_fuse_mkdir");
	return;
};


