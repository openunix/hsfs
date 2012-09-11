/**
 * hsx_fuse_link
 * sunxiaobin
 * 2012/09/06
 */

#include <errno.h>
#include "hsfs.h"
#include "hsx_fuse.h"


void hsx_fuse_link(fuse_req_t req, fuse_ino_t ino, fuse_ino_t newparent, const char *newname)
{
	DEBUG_IN("%s","()");
	struct fuse_entry_param *e=NULL;
	// to get the super block
	struct hsfs_super *hsfs_sb=fuse_req_userdata(req);
	struct hsfs_inode *parent=hsx_fuse_iget(hsfs_sb,newparent);
	struct hsfs_inode *inop=hsx_fuse_iget(hsfs_sb,ino);
	struct hsfs_inode *newinode=NULL;
	int err= hsi_nfs3_link(inop, parent, &newinode, newname);
	if(!err){
		e=(struct fuse_entry_param *)malloc(sizeof(struct fuse_entry_param));
		if(!e){
			err=-ENOMEM;
			fuse_reply_err(req,err);
			goto out;
		}
		else{
			hsx_fuse_fill_reply(newinode,e);
			fuse_reply_entry(req, e);
			free(e);
			goto out;
		}
	}
	else
		fuse_reply_err(req,err);
out:
	DEBUG_OUT("  err%d",err);
	return;
}

