/*
 *hsx_fuse_readdir.c
 */

#include <fuse/fuse_lowlevel.h>
#include "hsi_nfs3.h"

void hsx_fuse_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	struct hsfs_readdir_ctx *hrc = NULL;
	int err = 0;

	hrc = (struct hsfs_readdir_ctx*)malloc(sizeof(struct hsfs_readdir_ctx));
	if (hrc == NULL)
	{
		printf("No menory: hrc.");
		err = 12;
		goto out;
	}
	memset(hrc, 0, sizeof(struct hsfs_readdir_ctx));
	fi->fh = (size_t)hrc;
	fuse_reply_open(req, fi);
	free(hrc);

out:	
	
	return;
}

