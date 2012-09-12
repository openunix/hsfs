/**
 * hsx_fuse_open
 * liuyoujin
 */
#include <sys/errno.h>
#include "hsi_nfs3.h"
#include "log.h"

#define FI_FH_LEN    10

extern void hsx_fuse_open (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	DEBUG_IN (" fi->flags:%d",fi->flags);
	int err=0;
	uint64_t fh =(uint64_t) malloc (FI_FH_LEN);
	if (!fh){
		err = ENOMEM;
		ERR ("%d\n",err);
		fuse_reply_err(req, err);

	}
	else {
		fi->fh = fh;
		fuse_reply_open(req, fi);
	}
	DEBUG_OUT(" fh:%lu",fh);	
}

