/**
 * hsx_fuse_release
 * liuyoujin
 */

#include <sys/errno.h>
#include "hsi_nfs3.h"
#include "log.h"

void hsx_fuse_release (fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	DEBUG_IN ("ino (%lu) , fi->flags:%d",ino, fi->flags);
	if (fi->fh != 0){
		free ((void *)fi->fh);
	}
	fuse_reply_err(req, 0);
	DEBUG_OUT(" fi->flags:%d",fi->flags);
}
