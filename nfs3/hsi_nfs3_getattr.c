#include "hsi_nfs3.h"

int hsi_nfs3_getattr(struct hsfs_inode *inode)
{
	int err = 0;
	nfs_fh3 fh;
	CLIENT *clntp = NULL;
	struct timeval to;
	struct getattr3res res;
	enum clnt_stat st;
	struct fattr3 *attr = NULL;
	
	DEBUG_IN("%s", "Enter hsi_nfs3_getattr().\n");

	clntp= inode->sb->clntp;
	to.tv_sec = inode->sb->timeo / 10;
	to.tv_usec = (inode->sb->timeo % 10) * 100000;
	fh.data.data_len = inode->fh.data.data_len;
	fh.data.data_val = inode->fh.data.data_val;
	memset(&res, 0, sizeof(res));
	st = clnt_call(clntp, NFSPROC3_GETATTR, (xdrproc_t)xdr_nfs_fh3, 
		(caddr_t)&fh, (xdrproc_t)xdr_getattr3res, (caddr_t)&res,
		to);
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
	/* fill return value to fattr3 attr in struct hsfs_inode */
	attr = &res.getattr3res_u.attributes;
	inode->attr.type = attr->type;
	inode->attr.mode = attr->mode;
	inode->attr.nlink = attr->nlink;
	inode->attr.uid = attr->uid;
	inode->attr.gid = attr->gid;
	inode->attr.size = attr->size;
	inode->attr.used = attr->used;
	inode->attr.rdev.major = attr->rdev.major;
	inode->attr.rdev.minor = attr->rdev.minor;
	inode->attr.fsid = attr->fsid;
	inode->attr.fileid = attr->fileid;
	inode->attr.atime.seconds = attr->atime.seconds;
	inode->attr.atime.nseconds = attr->atime.nseconds;
	inode->attr.mtime.seconds = attr->mtime.seconds;
	inode->attr.mtime.nseconds = attr->mtime.nseconds;
	inode->attr.ctime.seconds = attr->ctime.seconds;
	inode->attr.ctime.nseconds = attr->ctime.nseconds;
 out_1:
	clnt_freeres(clntp, (xdrproc_t)xdr_getattr3res, (char *)&res);
 out_2:
	DEBUG_OUT("Leave hsi_nfs3_getattr() with errno %d.\n", err);
	return err;
}
