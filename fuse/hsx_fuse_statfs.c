#include <sys/errno.h>
#include "hsi_nfs3.h"
#include "hsx_fuse.h"

void hsx_fuse_statfs(fuse_req_t req, fuse_ino_t ino)
{
	struct statvfs stbuf;
	struct hsfs_super *super = NULL;
	struct hsfs_inode *root = NULL;
	struct hsfs_super *sp = NULL;
	int err=0;
	DEBUG_IN(" err:%d", err);

	super = fuse_req_userdata(req);
	root = super->root;
	sp = root->sb;
	err = hsi_nfs3_statfs(root);
	if (err){
		fuse_reply_err (req, err);
		goto out;
	}
	err = hsi_super2statvfs(sp, &stbuf);
	if (err){
		err = EINVAL;
		fuse_reply_err (req, err);
		goto out;
	}
	fuse_reply_statfs (req, &stbuf);
out:
	DEBUG_OUT (" err:%d",err);
	return;

}


int hsi_super2statvfs(struct hsfs_super *sp, struct statvfs *stbuf)
{
	unsigned long block_res = 0;
	unsigned char block_bits = 0;

	if (sp->bsize_bits == 0){
		ERR ("hsfs_super->bsize_bits is 0!");
		goto pout;
	}
	block_bits  = sp->bsize_bits;
	block_res = (1 << block_bits)-1;
	if (sp->bsize == 0){
		ERR ("hsfs_super->bsize is 0!\n");
		goto pout;
	}
	stbuf->f_bsize = sp->bsize;
	stbuf->f_frsize = sp->bsize;
	stbuf->f_blocks = (sp->tbytes+block_res)>>block_bits;
	stbuf->f_bfree = (sp->fbytes+block_res)>>block_bits;
	stbuf->f_bavail = (sp->abytes+block_res)>>block_bits;
	stbuf->f_files = sp->tfiles;
	stbuf->f_ffree = sp->ffiles;
	stbuf->f_favail = sp->afiles;
	stbuf->f_fsid = 0;
	if (sp->namlen == 0){
		ERR("super->namemax is 0!\n");
		goto pout;
	}
	stbuf->f_flag = sp->flags;
	stbuf->f_namemax = sp->namlen;
	return 0;
pout:
	return 1;
}
