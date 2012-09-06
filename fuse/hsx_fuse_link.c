/**
 * hsx_fuse_link
 * sunxiaobin
 * 2012/09/06
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "hsx_fuse.h"
#include "hsi_nfs3.h"


void hsx_fuse_link(fuse_req_t req, fuse_ino_t ino, fuse_ino_t newparent, const char *newname)
{
	struct fuse_entry_param *e=NULL;
	// to get the super block
	struct hsfs_super *hsfs_sb=fuse_req_userdata(req);
	struct hsfs_inode *parent=hsx_fuse_iget(hsfs_sb,newparent);
	struct hsfs_inode *ino=hsx_fuse_iget(hsfs_sb,ino);
	struct hsfs_inode *newinode=NULL;
	int err=hsi_nfs3_link(ino, parent, &newinode, newname);
	if(!err){
		e=(struct fuse_entry_param *)malloc(sizeof(struct fuse_entry_param));
		if(!e){
			err=ENOMEM;
			goto out;
		}
		hsx_fuse_fill_reply(newinode,&e);
		fuse_reply_entry(req, e);
		free(e);
	}
	else
		fuse_reply_err(req,err);
out:
	return;
}

