#include "hsi_nfs3.h"

int hsi_nfs3_getattr(struct hsfs_inode *inode)
{
	int err = 0;
	nfs_fh3 fh;
	CLIENT *clntp;
	struct timeval to = {120, 0};
	struct getattr3res res;
	enum clnt_stat st;
	
	DEBUG_IN("%s", "Enter hsi_nfs3_getattr().\n");

	clntp= inode->sb->clntp;
	fh.data.data_len = inode->fh.data.data_len;
	fh.data.data_val = inode->fh.data.data_val;
	memset(&res, 0, sizeof(res));
	st = clnt_call(clntp, 1, (xdrproc_t)xdr_nfs_fh3, (caddr_t)&fh, 
		       (xdrproc_t)xdr_getattr3res, (caddr_t)&res, to);
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
	/* fill return value to fattr3 attr in struct hsfs_inode */
	inode->attr.type = res.getattr3res_u.attributes.type;
	inode->attr.mode = res.getattr3res_u.attributes.mode;
	inode->attr.nlink = res.getattr3res_u.attributes.nlink;
	inode->attr.uid = res.getattr3res_u.attributes.uid;
	inode->attr.gid = res.getattr3res_u.attributes.gid;
	inode->attr.size = res.getattr3res_u.attributes.size;
	inode->attr.used = res.getattr3res_u.attributes.used;
	inode->attr.rdev.major = res.getattr3res_u.attributes.rdev.major;
	inode->attr.rdev.minor = res.getattr3res_u.attributes.rdev.minor;
	inode->attr.fsid = res.getattr3res_u.attributes.fsid;
	inode->attr.fileid = res.getattr3res_u.attributes.fileid;
	inode->attr.atime.seconds = res.getattr3res_u.attributes.atime.seconds;
	inode->attr.atime.nseconds = res.getattr3res_u.attributes.atime.nseconds;
	inode->attr.mtime.seconds = res.getattr3res_u.attributes.mtime.seconds;
	inode->attr.mtime.nseconds = res.getattr3res_u.attributes.mtime.nseconds;
	inode->attr.ctime.seconds = res.getattr3res_u.attributes.ctime.seconds;
	inode->attr.ctime.nseconds = res.getattr3res_u.attributes.ctime.nseconds;
 out:
	DEBUG_OUT("Leave hsi_nfs3_getattr() with errno %d.\n", err);
	clnt_freeres(clntp, (xdrproc_t)xdr_getattr3res, (char *)&res);
	return err;
}
