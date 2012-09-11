#include <stdio.h>
#include <linux/fs.h>
#include <errno.h>
#include "log.h"
#include "hsfs.h"
#include "hsx_fuse.h"
#include "hsi_nfs3.h"
void hsx_fuse_access(fuse_req_t req, fuse_ino_t ino, int mask)
{
	DEBUG_IN("%s", "We now come into hsx_fuse_access()\n");
	struct hsfs_inode *hi = NULL;
	struct hsfs_super *hs = NULL;
	int mode = 0;
	int err = 0xFFFFFFFF;

	hs = fuse_req_userdata(req);
	if (NULL == hs) {
		ERR("ERROR occur while getting <struct hsfs_super>" 
			"in <hsx_fuse_access>!\n");
		err = EIO;
		goto out;
	}
	hi = hsx_fuse_iget(hs, ino);
	if (NULL == hi) {
		ERR("ERROR occur while getting <struct hsfs_inode>"
			"in <hsx_fuse_access>!\n");
		err = EIO;
		goto out;
	}

	if (mask & MAY_READ) {
		mode |= ACCESS3_READ;
	}
	if (S_ISDIR(hi->attr.type)) {
		if (mask & MAY_WRITE) {
			mode |= ACCESS3_MODIFY | ACCESS3_EXTEND 
				| ACCESS3_DELETE;
		}
		if (mask & MAY_EXEC) {
			mode |= ACCESS3_LOOKUP;
		}
	} else {
		if (mask & MAY_WRITE) {
			mode |= ACCESS3_MODIFY | ACCESS3_EXTEND;
		}
		if (mask & MAY_EXEC) {
			mode |= ACCESS3_EXECUTE;
		}
	}

	err = hsi_nfs3_access(hi, mode);
	
out:
	if (NULL != hs) {
		hs = NULL;
	}
	if (NULL != hi) {
		hi = NULL;
	}
	
	fuse_reply_err(req, err);
DEBUG_OUT("%s", "Out of hsx_fuse_access()\n");
}

