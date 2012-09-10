/*
 * hsx_fuse_readlink
 * xuehaitao
 * 2012.9.6
 */
#include "hsi_nfs3.h"
#include "hsfs.h"
#include "hsx_fuse.h"
#include "log.h"

void hsx_fuse_readlink(fuse_req_t req, fuse_ino_t ino)
{
	int st = 0;
        int err = 0;
	struct hsfs_inode *hi;
	struct hsfs_super *hi_sb;
	const char *link = NULL;
	hi = (struct hsfs_inode*)malloc(sizeof(struct hsfs_inode));
	hi_sb = (struct hsfs_super*)malloc(sizeof(struct hsfs_super));
	memset(&hi, 0, sizeof(struct hsfs_inode));
	memset(&hi_sb, 0, sizeof(struct hsfs_super));
	hi->sb = hi_sb;
	hi->sb = fuse_req_userdata(req);
	hi = hsx_fuse_iget(hi->sb, ino);
	DEBUG_IN("%s\n","THE HSX_FUSE_READLINK.");
	st = hsi_nfs3_readlink(hi, link);
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
	free(hi);
	free(hi_sb);
	if(st != 0){
		fuse_reply_err(req, err);
	}
	DEBUG_OUT("THE HSX_FUSE_READLINK FAILED. %d\n", err);
	return;
}
