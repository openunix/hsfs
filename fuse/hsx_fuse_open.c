/**
 * hsx_fuse_open
 * liuyoujin
 */
#include <sys/errno.h>
#include "hsi_nfs3.h"
#include "log.h"

#define FI_FH_LEN    10

void hsx_fuse_open (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	int err=0;
	uint64_t fh =(uint64_t) malloc (FI_FH_LEN);
	DEBUG_IN ("ino : (%lu)  fi->flags:%d",ino, fi->flags);
	if (!fh){
		err = ENOMEM;
		ERR ("malloc failed:%d\n",err);
		fuse_reply_err(req, err);

	}
	else {
		fi->fh = fh;
		fuse_reply_open(req, fi);
	}
	DEBUG_OUT(" fh:%lu",fh);	
}

