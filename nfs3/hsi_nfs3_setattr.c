#include <errno.h>
#include "hsi_nfs3.h"

int hsi_nfs3_setattr(struct hsfs_inode *inode, struct hsfs_sattr *attr)
{
	int err = 0;
	CLIENT *clntp = NULL;
	struct setattr3args args;
	struct wccstat3 res;
	struct fattr3 *fattr = NULL;
	
	DEBUG_IN("%s", "\n");

	clntp= inode->sb->clntp;
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
	err = hsi_nfs3_clnt_call(inode->sb, clntp, NFSPROC3_SETATTR,
			(xdrproc_t)xdr_setattr3args, (caddr_t)&args,
			(xdrproc_t)xdr_wccstat3, (caddr_t)&res);
	if (err) 
		goto out_no_free;

	if (NFS3_OK != res.status) {
		err = hsi_nfs3_stat_to_errno(res.status);
		ERR("RPC Server returns failed status : %d.\n", err);
		goto out;
	}
	if (!res.wccstat3_u.wcc.after.present) {
		err = EAGAIN;
		ERR("Try again to set file attributes.\n");
		goto out;
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
 out:
	clnt_freeres(clntp, (xdrproc_t)xdr_wccstat3, (char *)&res);
 out_no_free:
	DEBUG_OUT("with errno : %d.\n", err);
	return err;
}
