#include <errno.h>
#include "hsi_nfs3.h"

int hsi_nfs3_setattr(struct hsfs_inode *inode, struct hsfs_sattr *attr)
{
	int err = 0;
	CLIENT *clntp = NULL;
	struct timeval to;
	struct setattr3args args;
	struct wccstat3 res;
	enum clnt_stat st;
	struct fattr3 *fattr = NULL;
	
	DEBUG_IN("%s", "Enter hsi_nfs3_setattr().\n");

	clntp= inode->sb->clntp;
	to.tv_sec = inode->sb->timeo / 10;
	to.tv_usec = (inode->sb->timeo % 10) * 100000;
	memset(&args, 0, sizeof(args));
	args.object.data.data_len = inode->fh.data.data_len;
      	args.object.data.data_val = inode->fh.data.data_val;
	if (S_ISSETMODE(attr->valid)) {
		args.new_attributes.mode.set = TRUE;
		args.new_attributes.mode.set_uint32_u.val = attr->mode;
	}
	else
		args.new_attributes.mode.set = FALSE;	
	       
	if (S_ISSETUID(attr->valid)) {
		args.new_attributes.uid.set = TRUE;
		args.new_attributes.uid.set_uint32_u.val = attr->uid;
	}
	else
		args.new_attributes.uid.set = FALSE;
	
	if (S_ISSETGID(attr->valid)) {
		args.new_attributes.gid.set = TRUE;
		args.new_attributes.gid.set_uint32_u.val = attr->gid;
        }
        else
		args.new_attributes.gid.set = FALSE;

	if (S_ISSETSIZE(attr->valid)) {
		args.new_attributes.size.set = TRUE;
		args.new_attributes.size.set_uint64_u.val = attr->size;
        }
        else
		args.new_attributes.size.set = FALSE;

	if (S_ISSETATIME(attr->valid)) {
		args.new_attributes.atime.set = SET_TO_CLIENT_TIME;
		args.new_attributes.atime.set_time_u.time.seconds = attr->atime.tv_sec;
		args.new_attributes.atime.set_time_u.time.nseconds = attr->atime.tv_nsec;
        }
	else
		args.new_attributes.atime.set = DONT_CHANGE;
	
	if (S_ISSETMTIME(attr->valid)) {
        	args.new_attributes.mtime.set = SET_TO_CLIENT_TIME;
		args.new_attributes.mtime.set_time_u.time.seconds = attr->mtime.tv_sec;
		args.new_attributes.mtime.set_time_u.time.nseconds = attr->mtime.tv_nsec;
        }
        else
		args.new_attributes.mtime.set = DONT_CHANGE;

	if (S_ISSETATIMENOW(attr->valid)) 
		args.new_attributes.atime.set = SET_TO_SERVER_TIME;

	if (S_ISSETMTIMENOW(attr->valid))
		args.new_attributes.mtime.set = SET_TO_SERVER_TIME;
	
	args.guard.check = FALSE;
	memset(&res, 0, sizeof(res));
	st = clnt_call(clntp, NFSPROC3_SETATTR, (xdrproc_t)xdr_setattr3args, 
		(caddr_t)&args, (xdrproc_t)xdr_wccstat3, (caddr_t)&res, to);
	if (st) {
		err = hsi_rpc_stat_to_errno(clntp);
		ERR("Call RPC Server failure: %d.\n", err);
		goto out_2;
	}
	if (NFS3_OK != res.status) {
		err = hsi_nfs3_stat_to_errno(res.status);
		ERR("RPC Server returns failed status : %d.\n", err);
		goto out_1;
	}
	if (!res.wccstat3_u.wcc.after.present) {
		err = EAGAIN;
		ERR("Try again to set file attributes.\n");
		goto out_1;
	}
	fattr = &res.wccstat3_u.wcc.after.post_op_attr_u.attributes;
	inode->attr.type = fattr->type;
	inode->attr.mode = fattr->mode;
	inode->attr.nlink = fattr->nlink;
	inode->attr.uid = fattr->uid;
	inode->attr.gid = fattr->gid;
	inode->attr.size = fattr->size;
	inode->attr.used = fattr->used;
	inode->attr.rdev.major = fattr->rdev.major;
	inode->attr.rdev.minor = fattr->rdev.minor;
	inode->attr.fsid = fattr->fsid;
	inode->attr.fileid = fattr->fileid;
 
	inode->attr.atime.seconds = fattr->atime.seconds;
	inode->attr.atime.nseconds = fattr->atime.nseconds;
	inode->attr.mtime.seconds = fattr->mtime.seconds;
	inode->attr.mtime.nseconds = fattr->mtime.nseconds;
	inode->attr.ctime.seconds = fattr->ctime.seconds;
	inode->attr.ctime.nseconds = fattr->ctime.nseconds;
 out_1:
	clnt_freeres(clntp, (xdrproc_t)xdr_wccstat3, (char *)&res);
 out_2:
	DEBUG_OUT("Leave hsi_nfs3_setattr() with errno : %d.\n", err);
	return err;
}
