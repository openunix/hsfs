/*
 * hsx_fuse_readlink
 * xuehaitao
 * 2012.9.6
 */
#include "hsi_nfs3.h"
#include "hsfs.h"
#include "hsx_fuse.h"
#include "log.h"

void hsx_fuse_readlink(fuse_req_t req, fuse_ino_t ino){

	int st = 0;
	int err = 0;
	struct hsfs_inode *hi = NULL;
	struct hsfs_super *hi_sb = NULL;
	char *link = NULL;
	
	hi_sb = fuse_req_userdata(req);
	hi = hsx_fuse_iget(hi_sb, ino);
	DEBUG_IN("%s\n","THE HSX_FUSE_READLINK.");
	st = hsi_nfs3_readlink(hi,&link);
	if(st != 0){
		err = st;
		goto out;
	}
	st = fuse_reply_readlink(req, link);
	if(st != 0){
		err = st;
		goto out;
	}

out:
	free(link);
	if(st != 0){
		fuse_reply_err(req, err);
	}
	DEBUG_OUT("THE HSX_FUSE_READLINK FAILED. %d\n", err);
	return;
}
