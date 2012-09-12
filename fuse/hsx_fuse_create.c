#include <errno.h>
#include "hsfs.h"
#include "hsx_fuse.h"
#include "hsi_nfs3.h"
#include "log.h"


void hsx_fuse_create(fuse_req_t req, fuse_ino_t parent, const char *name,
			mode_t mode, struct fuse_file_info *fi)
{
	DEBUG_IN("%s", "Now we come into hsx_fuse_create()\n");
	struct hsfs_inode *hi = NULL;
	struct hsfs_inode *newhi = NULL;
	struct hsfs_super *hs = NULL;
	struct fuse_entry_param *e = NULL;
	int mymode = 0;
	int err =0;
	newhi = (struct hsfs_inode *)malloc(sizeof(struct hsfs_inode));
	e = (struct fuse_entry_param *)malloc(sizeof(struct fuse_entry_param));
	if (NULL == e){
		ERR("ERROR occur while getting new <struct fuse_entry_param>"
			"in <hsx_fuse_create>!\n");
		err = EIO;
		goto out;
	}
	if (NULL == newhi){
		ERR("ERROR occur while getting new <struct hsfs_inode>"
			"in <hsx_fuse_create>!\n");
		err = EIO;
		goto out;
	}
	hs = fuse_req_userdata(req);
	if (NULL == hs) {
		ERR("ERROR occur while getting <struct hsfs_super>"
			"in <hsx_fuse_create>!\n");
		err = EIO;
		goto out;
	}
	hi = hsx_fuse_iget(hs, (uint64_t)parent);
	if (NULL == hi) {
		ERR("ERROR occur while getting <struct hsfs_super>"
			"in <hsx_fuse_create>!\n");
		err = EIO;
		goto out;
	}

	mymode = (mode & S_IRWXO) | ((mode & S_IRWXG)) 
			| ((mode & S_IRWXU));//identify the file mode
	if (fi->flags & O_EXCL) {
		mymode |= 0x2 << 16;
	} else if (fi->flags & O_TRUNC) {
		mymode |= 0x1 << 16;
	} else {
		mymode |= 0x0;
	}

	
	err = hsi_nfs3_create(hi, &newhi, name, mymode);
	if (0 == err) {
		hsx_fuse_fill_reply(newhi, e);
		fuse_reply_create(req, e, fi);
	} else {
		fuse_reply_err(req, err);
	}

out:
	if (NULL != hi) {
		hi = NULL;
	}
	if (NULL !=hs) {
		hs = NULL;
	}
	if (NULL != newhi) {
		newhi = NULL;
	}
	DEBUG_OUT("%s", "Out of hsx_fuse_create()\n");
}
