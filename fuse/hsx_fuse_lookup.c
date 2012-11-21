#include <errno.h>
#include "hsi_nfs3.h"
#include "hsx_fuse.h"

void  hsx_fuse_lookup(fuse_req_t req, fuse_ino_t ino, const char *name)
{
	struct  hsfs_super *sb;
	struct  hsfs_inode *parent;
	struct  hsfs_inode *child;
	struct  fuse_entry_param e;
	int err = 0;

	sb = fuse_req_userdata(req);
	DEBUG_IN("SB(%p), P_IN(%lu), Name(%s)", req, ino, name);

	if((parent=hsfs_ilookup(sb,ino)) == NULL)
	{
		err = ENOENT;
		goto out;
	}

	err = hsi_nfs3_lookup(parent,&child,name);

	if (err)
		goto out;
	if (!child){
		err = ENOMEM;
		goto out;
	}

	hsx_fuse_ref_inc(child, 1);
	hsx_fuse_fill_reply(child, &e);

	fuse_reply_entry(req, &e);
	DEBUG_OUT("with new Inode(%p:%lu)", child, child->ino);
	return ;
out:
	fuse_reply_err(req,err);
	DEBUG_OUT(" with errno %d",err);
}
