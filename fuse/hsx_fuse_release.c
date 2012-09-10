/**
 * hsx_fuse_release
 * liuyoujin
 */
#include <errno.h>
#include <stdio.h>
#include <fuse/fuse_lowlevel.h>
#include "log.h"
extern void hsx_fuse_release (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
DEBUG_IN ("INODE",ino);
	if (fi->fh != NULL){
		free (fi->fh);
	}
	fuse_reply_err(req, 0);
DEBUG_OUT("INODE",ino);
}
