/*
 *hsx_fuse_readdir.c
 */

#include <fuse/fuse_lowlevel.h>
#include "hsi_nfs3.h"

void hsx_fuse_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	struct hsfs_readdir_ctx *hrc = NULL;

	DEBUG_IN("%s.","hsx_fuse_opendir");
	hrc = (struct hsfs_readdir_ctx*)malloc(sizeof(struct hsfs_readdir_ctx));
	if (hrc == NULL)
	{
		ERR("opendir_hrc menory leak.");
		goto out;
	}

	memset(hrc, 0, sizeof(struct hsfs_readdir_ctx));
	fi->fh = (size_t)hrc;
	fuse_reply_open(req, fi);
	free(hrc);

out:	
	DEBUG_OUT("%s.","hsx_fuse_opendir");
	return;
}

