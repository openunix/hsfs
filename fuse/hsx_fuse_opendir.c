/*
 *hsx_fuse_opendir.c
 */

#include <errno.h>
#include <fuse/fuse_lowlevel.h>
#include "hsi_nfs3.h"

void hsx_fuse_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	struct hsfs_super *sb = NULL;
	struct hsfs_inode *parent = NULL;
	int err = 0;

	DEBUG_IN("%s.","hsx_fuse_opendir");
	sb = fuse_req_userdata(req);
	if(!sb){
		ERR("%s gets hsfs_super fails \n", progname);
		err = ENOENT;
		goto out;
	}

	parent = hsx_fuse_iget(sb, ino);
	if(!parent){
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

