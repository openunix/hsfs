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
	struct fuse_entry_param *e=NULL;
	// get the super block of hsfs
	struct hsfs_super *hsfs_sb=fuse_req_userdata(req);
	// get the inode of parent 
	struct hsfs_inode *parentp=hsx_fuse_iget(hsfs_sb,parent);
	if(!parentp){
		ERR("%s gets hsfs_inode fails \n",progname);
		err=ENOENT;
		goto out;	
	}
	struct hsfs_inode *newinode=NULL;
	err=hsi_nfs3_mknod(parentp,&newinode,name,mode,rdev);
	if(!err){
		e=(struct fuse_entry_param *)malloc(sizeof(struct
                                               fuse_entry_param));
		if(!e){
			err=ENOMEM;
			fuse_reply_err(req,err);
			goto out;
		}
		else{
			hsx_fuse_fill_reply(newinode,e);
			fuse_reply_entry(req,e);	
			free(e);
			goto out;
		}
	}
	else
	fuse_reply_err(req,err);
out:
	DEBUG_OUT(" err %d",err);
	return;
}
