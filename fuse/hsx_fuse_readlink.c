/*
 * hsx_fuse_readlink
 * xuehaitao
 * 2012.9.6
 */
#include "hsi_nfs3.h"
#include "hsx_fuse.h"
#include "log.h"
#include <errno.h>

void hsx_fuse_readlink(fuse_req_t req, fuse_ino_t ino)
{
	int st = 0;
	int err = 0;
	struct hsfs_inode *hi = NULL;
	struct hsfs_super *hi_sb = NULL;
	char *link = NULL;
	DEBUG_IN("%s\n","THE HSX_FUSE_READLINK.");

	hi_sb = fuse_req_userdata(req);
	if(!hi_sb){
		ERR("%s gets inode->sb fails \n", progname);
		err = ENOENT;
		goto out;
	}
	hi = hsx_fuse_iget(hi_sb, ino);
	if(!hi){
		ERR("%s gets inode fails \n", progname);
		err = ENOENT;
		goto out;
	}
	st = hsi_nfs3_readlink(hi,&link);
	if(st != 0){
		err = st;
		goto out;
	}
	fuse_reply_readlink(req, link);

out:
	if(link != NULL){
		free(link);
	}
	if(st != 0){
		fuse_reply_err(req, err);
	}
	DEBUG_OUT("THE HSX_FUSE_READLINK RETURN WITH ERRNO %d\n", err);
	return;
}
