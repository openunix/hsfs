/*
 *hsx_fuse_opendir.c
 */

#include <errno.h>
#include <fuse/fuse_lowlevel.h>
#include "hsi_nfs3.h"

void hsx_fuse_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	struct hsfs_super *hs = NULL;
	struct hsfs_inode *hi = NULL;
	int err = 0;

	DEBUG_IN("%s.","hsx_fuse_opendir");
	hs = fuse_req_userdata(req);
	if(!hs){
		ERR("%s gets hsfs_super fails \n", progname);
		err = ENOENT;
		goto out;
	}

	hi = hsx_fuse_iget(hs, ino);
	if(!hi){
		ERR("%s gets file handle fails \n", progname);
		err = ENOENT;
		goto out;
	}

	fuse_reply_open(req, fi);

out:	
	if(err != 0)
	{
		fuse_reply_err(req, err);
	}

	DEBUG_OUT("Open dir fails: %d.", err);
	return;
}

