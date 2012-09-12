#include <errno.h>
#include "hsi_nfs3.h"

/* Defined in fuse/fuse_lowlevel.h */
#ifndef FUSE_SET_ATTR_MODE 
#define FUSE_SET_ATTR_MODE       (1 << 0)
#endif

#ifndef FUSE_SET_ATTR_UID
#define FUSE_SET_ATTR_UID        (1 << 1)
#endif

#ifndef FUSE_SET_ATTR_GID
#define FUSE_SET_ATTR_GID        (1 << 2)
#endif

#ifndef FUSE_SET_ATTR_SIZE
#define FUSE_SET_ATTR_SIZE       (1 << 3)
#endif

#ifndef FUSE_SET_ATTR_ATIME
#define FUSE_SET_ATTR_ATIME      (1 << 4)
#endif

#ifndef FUSE_SET_ATTR_MTIME
#define FUSE_SET_ATTR_MTIME      (1 << 5)
#endif

#ifndef FUSE_SET_ATTR_ATIME_NOW
#define FUSE_SET_ATTR_ATIME_NOW  (1 << 7)
#endif

#ifndef FUSE_SET_ATTR_MTIME_NOW
#define FUSE_SET_ATTR_MTIME_NOW  (1 << 8)
#endif

/* Custom defined */
#define S_ISSETMODE(to_set)       ((to_set) & FUSE_SET_ATTR_MODE)
#define S_ISSETUID(to_set)        ((to_set) & FUSE_SET_ATTR_UID)
#define S_ISSETGID(to_set)        ((to_set) & FUSE_SET_ATTR_GID)
#define S_ISSETSIZE(to_set)       ((to_set) & FUSE_SET_ATTR_SIZE)
#define S_ISSETATIME(to_set)      ((to_set) & FUSE_SET_ATTR_ATIME)
#define S_ISSETMTIME(to_set)      ((to_set) & FUSE_SET_ATTR_MTIME)
#define S_ISSETATIMENOW(to_set)   ((to_set) & FUSE_SET_ATTR_ATIME_NOW)
#define S_ISSETMTIMENOW(to_set)   ((to_set) & FUSE_SET_ATTR_MTIME_NOW)


int hsi_nfs3_setattr(struct hsfs_inode *inode, struct hsfs_sattr *attr)
{
	int err = 0;
	CLIENT *clntp = NULL;
	struct timeval to = {120, 0};
	struct setattr3args args;
	struct wccstat3 res;
	enum clnt_stat st;
  
	DEBUG_IN("%s", "Enter hsi_nfs3_setattr().\n");

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
	st = clnt_call(clntp, 2, (xdrproc_t)xdr_setattr3args, (caddr_t)&args, 
		       (xdrproc_t)xdr_wccstat3, (caddr_t)&res, to);
	if (st) {
		err = hsi_rpc_stat_to_errno(clntp);
		ERR("Call RPC Server failure: %d.\n", err);
		goto out;
	}
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
	inode->attr.type = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.type;
	inode->attr.mode = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.mode;
	inode->attr.nlink = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.nlink;
	inode->attr.uid = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.uid;
	inode->attr.gid = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.gid;
	inode->attr.size = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.size;
	inode->attr.used = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.used;
	inode->attr.rdev.major = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.rdev.major;
	inode->attr.rdev.minor = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.rdev.minor;
	inode->attr.fsid = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.fsid;
	inode->attr.fileid = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.fileid;
 
	inode->attr.atime.seconds = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.atime.seconds;
	inode->attr.atime.nseconds = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.atime.nseconds;
	inode->attr.mtime.seconds = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.mtime.seconds;
	inode->attr.mtime.nseconds = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.mtime.nseconds;
	inode->attr.ctime.seconds = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.ctime.seconds;
	inode->attr.ctime.nseconds = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.ctime.nseconds;
	
 out:
	DEBUG_OUT("Leave hsi_nfs3_setattr() with errno : %d.\n", err);
	clnt_freeres(clntp, (xdrproc_t)xdr_wccstat3, (char *)&res);
	return err;
}
