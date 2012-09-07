#include <errno.h>
#include "hsfs.h"
#include "hsx_fuse.h"
#include "hsi_nfs3.h"

void  hsx_fuse_lookup(fuse_req_t req, fuse_ino_t ino, const char *name)
{
	struct  hsfs_super	*sb;
	struct  hsfs_inode	*parent;
	struct  hsfs_inode	*child;
	struct  fuse_entry_param	*e;
	int	err = 0;

	sb = fuse_req_userdata(req);
	if((parent=hsx_fuse_iget(sb,ino)) == NULL)
	{
		err = ENOTDIR;
		fuse_reply_err(req,err);
	}

	if(err=hsi_nfs3_lookup(parent,&child,name))
	{
		fuse_reply_err(req,err);
	}

	hsx_fuse_fill_reply(child,&e);

	fuse_reply_entry(req, e);

}
