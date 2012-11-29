#include <errno.h>
#include "hsfs.h"
#include "hsx_fuse.h"
#include "hsi_nfs3.h"

void
hsx_fuse_mknod(fuse_req_t req, fuse_ino_t parent, const char *name,
                 mode_t mode, dev_t rdev)
{
	DEBUG_IN("name to mknod: %s",name);
	int err=0;
	unsigned long ref;
	struct fuse_entry_param e;
	// get the super block of hsfs
	struct hsfs_super *hsfs_sb=fuse_req_userdata(req);
	// get the inode of parent 
	struct hsfs_inode *parentp=hsfs_ilookup(hsfs_sb,parent);
	if(!parentp){
		ERR("%s gets hsfs_inode fails \n",progname);
		err=ENOENT;
		goto out;	
	}
	struct hsfs_inode *newinode=NULL;
	err=hsi_nfs3_mknod(parentp,&newinode,name,mode,rdev);

	if (err)
		goto out;

	if(newinode == NULL) {
		err = ENOMEM;
		goto out;
	}

	ref = hsx_fuse_ref_xchg(newinode, 1);
	FUSE_ASSERT(ref == 0);	/* Should be a new one... */

	hsx_fuse_fill_reply(newinode, &e);
	fuse_reply_entry(req, &e);

	DEBUG_OUT("New indoe at %p", newinode);
	return;

out:
	fuse_reply_err(req,err);
	DEBUG_OUT(" err %d",err);
}
