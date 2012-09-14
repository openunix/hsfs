/*
 *hsx_fuse_readdir.c
 */

#include <errno.h>
#include "hsi_nfs3.h"
#define RPCCOUNT 8

void hsx_fuse_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                         struct fuse_file_info *fi)
{
	struct hsfs_readdir_ctx *hrc = NULL;
	struct hsfs_readdir_ctx *temp_ctx = NULL;
	struct hsfs_inode *hi = NULL;
	struct hsfs_super *hs = NULL;
	size_t maxcount = 0;
	size_t newlen = 0;
	char * buf = NULL;
	int err=0;

	DEBUG_IN("The size is 0x%x, the off is 0x%x.", (unsigned int)size, 
		 (unsigned int)off);
	buf = (char *) malloc(size);
	if( NULL == buf){
		err = ENOMEM;
		ERR("Buf memory leak is 0x%x.", err);
		goto out;
	}

	hrc = (struct hsfs_readdir_ctx*)malloc(sizeof(struct hsfs_readdir_ctx));
	if( NULL == hrc){
		err = ENOMEM;
		ERR("hrc memory leak is 0x%x", err);
		goto out;
	}

	hs = fuse_req_userdata(req);
	if(!hs){
		err = ENOENT;
		ERR("%s gets hsfs_super fails \n", progname);
		goto out;
	}

	memset(hrc, 0, sizeof(struct hsfs_readdir_ctx));
	maxcount = RPCCOUNT*size;
	hrc->off = off;
	hi = hsx_fuse_iget(hs,ino);
	if(!hi){
		err = ENOENT;
		ERR("%s gets file handle fails \n", progname);
		goto out;
	}

	err = hsi_nfs3_readdir(hi, hrc, maxcount);
	if(err)
	{
		ERR("Call hsi_nfs3_readdir failed 0x%x", err);
		goto out;
	}

	temp_ctx = hrc;
	temp_ctx = temp_ctx->next;
	while(temp_ctx!=NULL){
	  	size_t tmplen = newlen;
	  	newlen += fuse_add_direntry(req, buf + tmplen, size -tmplen, 
					temp_ctx->name, &temp_ctx->stbuf, 
							temp_ctx->off);
	  	if(newlen>size)
	    		break;
		  
		temp_ctx = temp_ctx->next;
		
	}

	fuse_reply_buf(req, buf, min(newlen, size));
	

out:	
	if(NULL != buf)
		free(buf);
	while(hrc != NULL){
		temp_ctx = hrc;
		hrc = hrc->next;
		free(temp_ctx->name);
		free(temp_ctx);
	}
	if(err != 0){
		fuse_reply_err(req, err);
	}
	
	DEBUG_OUT("err is 0x%x.", err);
		
	return;
}


