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

#include <errno.h>
#include "hsi_nfs3.h"

int hsi_nfs3_wcc2fattr(struct wcc_data *wcc, struct nfs_fattr *fattr)
{
	return hsi_nfs3_post2fattr(&(wcc->after), fattr);
}

int hsi_nfs3_setattr(struct hsfs_inode *inode, struct nfs_fattr *fattr, struct hsfs_iattr *attr)
{
	int err = 0;
	CLIENT *clntp = NULL;
	struct setattr3args args;
	struct wccstat3 res;
	
	DEBUG_IN("%s", "\n");

	clntp= inode->sb->clntp;
	memset(&args, 0, sizeof(args));
	
	hsi_nfs3_getfh3(inode, &args.object);

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

	nfs_init_fattr(fattr);
	if (!hsi_nfs3_wcc2fattr(&res.wccstat3_u.wcc, fattr)){
		err = EAGAIN;
		ERR("Try again to set file attributes.\n");
		goto out;
	}

 out:
	clnt_freeres(clntp, (xdrproc_t)xdr_wccstat3, (char *)&res);
 out_no_free:
	DEBUG_OUT("with errno : %d.\n", err);
	return err;
}
