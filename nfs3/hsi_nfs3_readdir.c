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

#include <hsfs/hsi_nfs.h>
#include "hsfs.h"
#include "hsi_nfs3.h"
#include "log.h"


static void __get_cookie_verf(struct hsfs_inode *inode, cookieverf3 *verf)
{
	memcpy(verf, &(NFS_I(inode)->cookieverf), NFS3_COOKIEVERFSIZE);
}

static void __set_cookie_verf(struct hsfs_inode *inode, cookieverf3 *verf)
{
	memcpy(&(NFS_I(inode)->cookieverf), verf, NFS3_COOKIEVERFSIZE);
}

static int __alloc_copy_name(struct hsfs_readdir_ctx **hrc, filename3 name)
{
	struct hsfs_readdir_ctx *ctx;
	int len, err = 0;

	ctx = malloc(sizeof(struct hsfs_readdir_ctx));
	if (!ctx){
		err = ENOMEM;
		goto out1;
	}
	
	len = strnlen(name, 255);
	ctx->name = malloc(len + 1);
	if (ctx->name == NULL){
		err = ENOMEM;
		goto out2;
	}

	ctx->name[len] = '\0';
	strncpy(ctx->name, name, len);

	ctx->next = NULL;
	ctx->inode = NULL;
	memset(&ctx->stbuf, 0, sizeof(struct stat));

	if (*hrc != NULL)
		(*hrc)->next = ctx;
	*hrc = ctx;
	
	return 0;

out2:
	free(ctx);
out1:
	return err;
}

static void __free_ctx(struct hsfs_readdir_ctx *ctx)
{
	struct hsfs_readdir_ctx *saved;
	struct hsfs_inode *new;

	while (ctx){
		if (ctx->name)
			free(ctx->name);
		new = ctx->inode;
		if (new)
			hsfs_iput(new);
		saved = ctx->next;
		free(ctx);
		ctx = saved;
	}
}

int hsi_nfs3_readdir(struct hsfs_inode *parent, unsigned int count, uint64_t cookie,
		     struct hsfs_readdir_ctx **ctx)
{
	struct hsfs_super *sb = parent->sb;
	CLIENT *clntp = sb->clntp;
	struct readdir3args args;
	struct readdir3res res;
	struct entry3 *entry;
	struct dirlist3 *dlist;
	struct nfs_fattr fattr;
	struct hsfs_readdir_ctx *temp_hrc = NULL;
	int err, ecount = 0;

	DEBUG_IN("(Cookie 0x%llx, Count %d)", cookie, count);
	
	memset(&res, 0, sizeof(res));
	args.cookie = cookie;
	__get_cookie_verf(parent, &args.cookieverf);
	args.count = count;
	hsi_nfs3_getfh3(parent, &args.dir);
	
	err = hsi_nfs3_clnt_call(sb, clntp, NFSPROC3_READDIR,
				 (xdrproc_t)xdr_readdir3args, (char *)&args,
				 (xdrproc_t)xdr_readdir3res, (char *)&res);
	if (err)
		goto out1;
	
	if (NFS3_OK != res.status) {
		ERR("Call NFS3 Server failure:(%d).\n", res.status);
		err = hsi_nfs3_stat_to_errno(res.status);
		goto out2;
	}

	nfs_init_fattr(&fattr);
	hsi_nfs3_post2fattr(&res.readdir3res_u.resok.dir_attributes, &fattr);
	err = nfs_refresh_inode(parent, &fattr);
	if (err)
		goto out2;
	__set_cookie_verf(parent, &res.readdir3res_u.resok.cookieverf);

	dlist = &res.readdir3res_u.resok.reply;
	entry = dlist->entries;
	*ctx = NULL;

	while(entry) {
		err = __alloc_copy_name(&temp_hrc, entry->name);
		if (err)
			goto out3;
		if (*ctx == NULL)
			*ctx = temp_hrc;
		temp_hrc->stbuf.st_ino = entry->fileid;
		temp_hrc->off = entry->cookie;
		entry = entry->nextentry;
		ecount++;
	}
	/* We ignore dlist->eof here because Fuse don't need it. */
	clnt_freeres(clntp, (xdrproc_t)xdr_readdir3res, (char *)&res);
	DEBUG_OUT("Success with %d entries", ecount);
	return 0;

out3:
	__free_ctx(*ctx);
	ctx = NULL;
out2:
	clnt_freeres(clntp, (xdrproc_t)xdr_readdir3res, (char *)&res);
out1:
	DEBUG_OUT("Failed with error %d", err);
	return err;
}

int hsi_nfs3_readdir_plus(struct hsfs_inode *parent, unsigned int count,
			  uint64_t cookie, struct hsfs_readdir_ctx **ctx, 
			  unsigned int maxcount)
{
	struct hsfs_super *sb = parent->sb;
	CLIENT *clntp = sb->clntp;
	struct hsfs_readdir_ctx *temp_hrc = NULL; 
	struct dirlistplus3 *dlist;
	struct entryplus3 *entry;
	struct readdirplus3args args;
	struct readdirplus3res res;
	int err, ecount = 0;
	struct nfs_fattr fattr;

	DEBUG_IN("P_I(%p), count(%d), cookie(0x%llx)", parent, count, 
		 cookie);

	memset(&res, 0, sizeof(res));
	args.cookie = cookie;
	__get_cookie_verf(parent, &args.cookieverf);
	args.dircount = count;
	hsi_nfs3_getfh3(parent, &args.dir);
		
	args.maxcount = maxcount;
	
	err = hsi_nfs3_clnt_call(sb, clntp, NFSPROC3_READDIRPLUS,
				(xdrproc_t)xdr_readdirplus3args, (char *)&args,
				(xdrproc_t)xdr_readdirplus3res, (char *)&res);
		if (err)
			goto out;

		if (NFS3_OK != res.status) {
			ERR("Call NFS3 Server failure:(%d).\n", res.status);
			err = hsi_nfs3_stat_to_errno(res.status);
			goto out2;
		}
	nfs_init_fattr(&fattr);
	hsi_nfs3_post2fattr(&res.readdirplus3res_u.resok.dir_attributes, &fattr);
	err = nfs_refresh_inode(parent, &fattr);
	if (err)
		goto out2;
	__set_cookie_verf(parent, &res.readdirplus3res_u.resok.cookieverf);

	dlist = &res.readdirplus3res_u.resok.reply;
	entry = dlist->entries;
	*ctx = NULL;

	while(entry){
		struct nfs_fh name_fh;
		struct nfs_fattr fattr;
		struct hsfs_inode *new;

		err = __alloc_copy_name(&temp_hrc, entry->name);
		if (err)
			goto out3;
		if(*ctx == NULL)
			*ctx = temp_hrc;
		temp_hrc->off = entry->cookie;
		temp_hrc->stbuf.st_ino = entry->fileid;
		
		nfs_init_fattr(&fattr);
		hsi_nfs3_post2fattr(&entry->name_attributes, &fattr);
		/* Actually, we should do lookup here..... */
		if (!entry->name_handle.present){
			err = ENOENT;
			goto out3;
		}
		nfs_copy_fh3(&name_fh,
			     entry->name_handle.post_op_fh3_u.handle.data.data_len, 
			     entry->name_handle.post_op_fh3_u.handle.data.data_val);
		new = hsi_nfs_fhget(sb, &name_fh, &fattr);
		if (IS_ERR(new)){
			err = PTR_ERR(new);
			goto out3;
		}
		temp_hrc->inode = new;
		ecount++;
		entry = entry->nextentry;
	}
	/* We ignore dlist->eof here because Fuse don't need it. */
		clnt_freeres(clntp, (xdrproc_t)xdr_readdirplus3res,(char*)&res);
	DEBUG_OUT("with %d entries returned.", ecount);
	return 0;
out3:
	__free_ctx(*ctx);
	*ctx = NULL;
out2:
	clnt_freeres(clntp, (xdrproc_t)xdr_readdirplus3res, (char *)&res);
out:
	DEBUG_OUT("with error %d.", err);
	return err;
}

#ifdef HSFS_NFS3_TEST
int main(int argc, char *argv[])
{
	char *svraddr = NULL;
	CLIENT *clntp = NULL;
	char *fpath = NULL;
	size_t fhLen = 0;
    	unsigned char *fhvalp = NULL;
	struct hysfattr3 *fattrp = NULL;
	struct hsfs_inode *parent = NULL;
	struct hsfs_readdir_ctx *hrc = NULL;
	struct rpc_err rerr;
	size_t *dircount = NULL;
	size_t maxcount = 0;
	struct timeval to = {10, 0};
	enum clnt_stat st;
	int err = 0;

	svraddr = argv[1];
	fpath = argv[2];
	
	err = map_path_to_nfs3fh(svraddr, fpath, &fhLen, &fhvalp);
	if(err)
	{
	  printf("Map path to nfs3fh failed.");
	  goto out;
	}
	
	clntp = clnt_create(svraddr, NFS_PROGRAM, NFS_V3, "tcp");
	if (NULL == clntp) {
		err = 3;
		printf("Create client failed.");
		goto out;
	} 
	
	parent = (struct hsfs_inode*)malloc(sizeof(struct hsfs_inode));
	hrc = (struct hsfs_readdir_ctx*)malloc(sizeof(struct hsfs_readdir_ctx));
	memset(hrc, 0, sizeof(struct hsfs_readdir_ctx));
	parent->sb = (struct hsfs_super*)malloc(sizeof(struct hsfs_super));
	parent->sb->clntp = clntp;
	parent->fh.data.data_val = fhvalp ;
	parent->fh.data.data_len = fhLen;
	maxcount = 8192;
	err = hsi_nfs3_readdir(parent, hrc, dircount, maxcount);
	if (err) {
		ERR("Call RPC Server failure:%s", clnt_sperrno(err));
                clnt_geterr(parent->sb->clntp, &rerr);
                err = rerr.re_errno;
		goto out;
	}
	
	while(hrc->next != NULL){	
		printf("the cookieverf is 0x%llx.\n", 
			*(long long *)hrc->next->cookieverf);
		printf("the name of entry is %s, the offset is %d.\n", 
			hrc->next->name, hrc->next->off);
		hrc = hrc->next;
	}

out:
	if (NULL != clntp)
		clnt_destroy(clntp);
	exit(err);
}
#endif /* HSFS_NFS3_TEST */
