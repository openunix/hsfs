#include <errno.h>
#include "hsfs.h"
#include "hsx_fuse.h"
#include "hsi_nfs3.h"
#include "log.h"

void hsx_fuse_create(fuse_req_t req, fuse_ino_t parent, const char *name,
		     mode_t mode, struct fuse_file_info *fi)
{
	struct hsfs_inode *hi = NULL;
	struct hsfs_inode *newhi = NULL;
	struct hsfs_super *hs = NULL;
	struct fuse_ctx * fc = NULL;
	struct fuse_entry_param e = {};
	int mymode = 0;
	int err =0;
	
	DEBUG_IN("%s%d", "In hsx_fuse_create(), with MODE = ", mode);
	
	hs = fuse_req_userdata(req);
	if (NULL == hs) {
		ERR("%s", "Getting hsfs_super fail!");
		err = EIO;
		goto out;
	}
	hi = hsx_fuse_iget(hs, (uint64_t)parent);
	if (NULL == hi) {
		ERR("%s", "Getting hsfs_inode fail!");
		err = EIO;
		goto out;
	}
	fc = (struct fuse_ctx *) fuse_req_ctx(req);
	if (NULL == fc) {
		ERR("%s", "Getting fuse_ctx fail!");
		goto out;
	}

	hi->attr.uid = fc->uid;
	hi->attr.gid = fc->gid;
	mymode = (mode & S_IRWXO) | ((mode & S_IRWXG)) 
			| (mode & S_IRWXU);//identify the file mode
	if (fi->flags & O_EXCL) {
		mymode |= 0x2 << 16;
	} else if (fi->flags & O_TRUNC) {
		mymode |= 0x1 << 16;
	} else {
		mymode |= 0x0;
	}

	err = hsi_nfs3_create(hi, &newhi, name, mymode);
	if (0 == err) {
		hsx_fuse_fill_reply(newhi, &e);
		fuse_reply_create(req, &e, fi);
	} else {
		fuse_reply_err(req, err);
	}
out:
	DEBUG_OUT("%s", "Out of hsx_fuse_create()\n");
}
