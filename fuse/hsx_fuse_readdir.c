/*
 * Copyright (C) 2012 Huang Yongsheng, Feng Shuo <steve.shuo.feng@gmail.com>
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

static void __free_ctx(struct hsfs_readdir_ctx *ctx,
		       struct hsfs_readdir_ctx *free_inode_after)
{
	struct hsfs_readdir_ctx *saved_ctx;
	int free_inode;

	if ((free_inode_after != NULL) && (free_inode_after == ctx))
		free_inode = 1;
	else
		free_inode = 0;

	while(ctx != NULL){
		if (!free_inode && free_inode_after != NULL){
			if (ctx == free_inode_after)
				free_inode = 1;
		}
		if (free_inode && (ctx->inode != NULL)){
			if (!hsx_fuse_ref_dec(ctx->inode, 1))
				hsfs_iput(ctx->inode);
		}
		if (ctx->name)
			free(ctx->name);
		saved_ctx = ctx->next;
		free(ctx);
		ctx = saved_ctx;
	}

}

#ifdef FUSE_CAP_READDIR_PLUS
void hsx_fuse_readdir_plus(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
			   struct fuse_file_info *fi)
{
	struct hsfs_readdir_ctx *saved_ctx, *ctx = NULL;
	struct hsfs_inode *parent;
	struct hsfs_super *sb;
	size_t res, len = 0;
	char * buf;
	int err, count = 0;

	DEBUG_IN("P_I(%lu), Size(%lld), Off(0x%llx)", ino, size, off);

	(void)fi;
	sb = fuse_req_userdata(req);
	FUSE_ASSERT(sb != NULL);

	parent = hsfs_ilookup(sb, ino);
	FUSE_ASSERT(parent != NULL);

	err = hsi_nfs3_readdir_plus(parent, size, off, &ctx, size);
	if(err)
		goto out1;
	saved_ctx = ctx;

	buf = (char *) malloc(size);
	if( NULL == buf){
		err = ENOMEM;
		goto out2;
	}
	while(ctx != NULL){
		struct fuse_entry_param e;

		hsx_fuse_fill_reply(ctx->inode, &e);
		res = fuse_add_direntry_plus(req, buf + len, size - len, ctx->name,
					     &e, ctx->off);
	  	if(res > size - len)
			break;
		else if (res == size - len){
			ctx = ctx->next;
			break;
		}
		count++;
	       	hsx_fuse_ref_inc(ctx->inode, 1);
		len += res;
		ctx = ctx->next;
	}

	if (!err)
		fuse_reply_buf(req, buf, len);
	
	free(buf);
out2:
	__free_ctx(saved_ctx, 0);
out1:
	if(err)
		fuse_reply_err(req, err);

	DEBUG_OUT("with %d, %d entries returned", err, count);
	return;
}
#endif

void hsx_fuse_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
		      struct fuse_file_info *fi)
{
	struct hsfs_readdir_ctx *saved_ctx, *ctx = NULL;
	struct hsfs_inode *parent;
	struct hsfs_super *sb;
	size_t res, len = 0;
	char * buf;
	int err, count = 0;

	DEBUG_IN("P_I(%lu), Size(%lld), Off(0x%llx)", ino, size, off);

	(void)fi;
	sb = fuse_req_userdata(req);
	FUSE_ASSERT(sb != NULL);

	parent = hsfs_ilookup(sb, ino);
	FUSE_ASSERT(parent != NULL);

	err = hsi_nfs3_readdir(parent, size, off, &ctx);
	if(err)
		goto out1;
	saved_ctx = ctx;

	buf = (char *) malloc(size);
	if( NULL == buf){
		err = ENOMEM;
		goto out2;
	}
	while(ctx != NULL){
		res = fuse_add_direntry(req, buf + len, size - len, ctx->name,
					&ctx->stbuf, ctx->off);
		/* From fuse doc, buf is not copied if res larger than
		 * requested */
	  	if(res >= size - len)
			break;
		len += res;
		ctx = ctx->next;
		count++;
	}
	/* If EOF, we will return an empty buffer here. */
	if (!err)
		fuse_reply_buf(req, buf, len);
	
	free(buf);
out2:
	__free_ctx(saved_ctx, 0);
out1:
	if(err)
		fuse_reply_err(req, err);

	DEBUG_OUT("with %d, %d entries returned.", err, count);
}
