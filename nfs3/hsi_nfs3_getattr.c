/*
 * Copyright (C) 2012 Zhao Yan, Feng Shuo <steve.shuo.feng@gmail.com>
 *
 * This file is part of HSFS.
 *
 * HSFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * HSFS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HSFS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "hsi_nfs3.h"

int hsi_nfs3_do_getattr(struct hsfs_super *sb, struct nfs_fh3 *fh,
			struct nfs_fattr *fattr, struct stat *st)
{
	struct getattr3res res;
	struct fattr3 *attr = NULL;
	int err;
	
	DEBUG_IN("(%p, %p, %p, %p)", sb, fh, fattr, st);

	memset(&res, 0, sizeof(res));
	err = hsi_nfs3_clnt_call(sb, sb->clntp, NFSPROC3_GETATTR,
				 (xdrproc_t)xdr_nfs_fh3, (caddr_t)fh,
				(xdrproc_t)xdr_getattr3res, (caddr_t)&res);
	if (err)
		goto out_no_free;

	if (NFS3_OK != res.status) {
		err = hsi_nfs3_stat_to_errno(res.status);
		ERR("RPC Server returns failed status : %d.\n", err);
		goto out;
	}
	
	attr = &res.getattr3res_u.attributes;
	if (fattr){
		hsi_nfs3_fattr2fattr(attr, fattr);
		DEBUG("get nfs_fattr(V:%x, U:%u, G:%u, S:%llu, I:%llu)",
		      fattr->valid, fattr->uid, fattr->gid, fattr->size, fattr->fileid);
	}
	if (st)
		hsi_nfs3_fattr2stat(attr, st);
	
 out:
	clnt_freeres(sb->clntp, (xdrproc_t)xdr_getattr3res, (char *)&res);
 out_no_free:
	DEBUG_OUT("with errno %d.\n", err);
	return err;
}

int hsi_nfs3_getattr(struct hsfs_inode *inode, struct stat *st)
{
	int err = 0;
	nfs_fh3 fh;
	struct nfs_fattr fattr;
	
	DEBUG_IN("(%p)", inode);

	hsi_nfs3_getfh3(inode, &fh);
	nfs_init_fattr(&fattr);
	err = hsi_nfs3_do_getattr(inode->sb, &fh, &fattr, st);
	if (err)
		goto out;

	err = nfs_refresh_inode(inode, &fattr);
out:
	DEBUG_OUT("(%d)", err);
	return err;
}
