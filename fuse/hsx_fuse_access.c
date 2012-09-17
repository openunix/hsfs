#include <stdio.h>
#include <linux/fs.h>
#include <errno.h>

#include "log.h"
#include "hsfs.h"
#include "hsx_fuse.h"
#include "hsi_nfs3.h"

void hsx_fuse_access(fuse_req_t req, fuse_ino_t ino, int mask)
{
	struct hsfs_inode *hi = NULL;
	struct hsfs_super *hs = NULL;
	int mode = 0;
	int err = 0;
	
	DEBUG_IN("INO = %lu, MASK = %d", ino, mask);
	
	hs = fuse_req_userdata(req);
	if (NULL == hs) {
		ERR("%s", "Getting hsfs_super fail!");
		err = EIO;
		goto out;
	}
	hi = hsx_fuse_iget(hs, ino);
	if (NULL == hi) {
		ERR("%s", "Getting hsfs_inode fail!");
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
	
	fuse_reply_err(req, err);
	
out:
	DEBUG_OUT("Out of hsx_fuse_access, with ERRNO = %d", err);
	return;
}

