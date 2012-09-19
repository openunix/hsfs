#include "hsi_nfs3.h"
#include "hsx_fuse.h"
#include "acl.h"


void hsx_fuse_setxattr(fuse_req_t req, fuse_ino_t ino, const char *name, const
			char *value, size_t size, int flags )
{
	struct hsfs_inode *hi = NULL;
	struct hsfs_super *sb = NULL;
	int type = 0;
	int err = 0;

	DEBUG_IN(" ino %lu.\n", ino);
	if ((sb = fuse_req_userdata(req)) == NULL) {
		ERR("ERR in fuse_req_userdata");
		goto out;
	}

	if ((hi = hsx_fuse_iget(sb, ino)) == NULL) {
		ERR("ERR in hsx_fuse_iget");
		goto out;
	}	
	
	if (strcmp(name, POSIX_ACL_XATTR_ACCESS) == 0)
		type = ACL_TYPE_ACCESS;
	else if (strcmp(name, POSIX_ACL_XATTR_DEFAULT) == 0)
		type = ACL_TYPE_DEFAULT;
	else
		goto out;
	
	err = hsi_nfs3_setxattr(hi, value, type, size);
	fuse_reply_err(req, err);

out:
	DEBUG_OUT("out,err: %d\n",err);
	fuse_reply_err(req, err);
	return;
}
