/*
 * Copyright (C) 2012 Feng Shuo <steve.shuo.feng@gmail.com>
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
#include <hsx_fuse.h>

#include <errno.h>
#include "hsi_nfs3.h"

/* XXX This is dirty. */
#define RPCCOUNT 4096

void __hsx_fuse_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
			struct fuse_file_info *fi, int plus)
{
	struct hsfs_readdir_ctx *saved_ctx, *ctx = NULL;
	struct hsfs_inode *parent;
	struct hsfs_super *sb;
	size_t res = 0;
	size_t len = 0;
	char * buf;
	int err = 0;

	DEBUG_IN("P_I(%lu), Size(%lld), Off(%llx)", ino, size, off);

	(void)fi;
	sb = fuse_req_userdata(req);
	FUSE_ASSERT(sb != NULL);

	parent = hsfs_ilookup(sb, ino);
	FUSE_ASSERT(parent != NULL);

	err = hsi_nfs3_readdir(parent, RPCCOUNT, off, &ctx, plus);
	if(err)
		goto out1;
	saved_ctx = ctx;

	buf = (char *) malloc(size);
	if( NULL == buf){
		err = ENOMEM;
		goto out2;
	}
	while(ctx != NULL){
		if (plus){
			struct fuse_entry_param e;

			hsx_fuse_fill_reply(ctx->inode, &e);
			res = fuse_add_direntry_plus(
				req, buf + len, size - len, ctx->name,
				&e, ctx->off);
		}
		else {
			res = fuse_add_direntry(
				req, buf + len, size - len, ctx->name,
				&ctx->stbuf, ctx->off);
		}

	  	if(res >= size - len)
			break;
		len += res;
		ctx = ctx->next;
	}

	if (!err)
		fuse_reply_buf(req, buf, len);
	
	free(buf);
out2:
	while(saved_ctx != NULL){
		ctx = saved_ctx;
		saved_ctx = ctx->next;
		if (err && ctx->inode != NULL){
			if (!hsx_fuse_ref_dec(ctx->inode, 1))
				hsfs_iput(ctx->inode);
		}
		free(ctx->name);
		free(ctx);
	}
out1:
	if(err)
		fuse_reply_err(req, err);

	DEBUG_OUT("err is 0x%x.", err);
	return;
}


#ifdef FUSE_CAP_READDIR_PLUS
void hsx_fuse_readdir_plus(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
			   struct fuse_file_info *fi)
{
	__hsx_fuse_readdir(req, ino, size, off, fi, 1);
}
#endif

void hsx_fuse_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
		      struct fuse_file_info *fi)
{
	__hsx_fuse_readdir(req, ino, size, off, fi, 0);
}
