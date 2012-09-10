/**
 * hsx_fuse_open
 * liuyoujin
 */
#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <rpc/rpc.h>
#include "log.h"
#include <fuse/fuse_lowlevel.h>
#define FI_FH_LEN    10
extern void hsx_fuse_open (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
DEBUG_IN ("INODE:%ul",(unsigned long)ino);
	uint64_t fh =(uint64_t) malloc (FI_FH_LEN);
	if (fh){
		fuse_reply_err(req, -ENOMEM);

	}
	else {
		fi->fh = fh;
		fuse_reply_open(req, fi);
	}
DEBUG_OUT("INODE:%ul",(unsigned long)ino);	
}

