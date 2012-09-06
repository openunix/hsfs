/**
 * hsx_fuse_release
 * liuyoujin
 */
#include <hsx_fuse.h>
#include <errno.h>
#include <stdio.h>
extern void hsx_fuse_release (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	if (fi->fh != NULL){
		free (fi->fh);
	}
	fuse_reply_open(req, 0);
}
