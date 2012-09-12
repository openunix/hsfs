/*
 *hsx_fuse_readdir.c
 */
#include <sys/errno.h>
#include <fuse/fuse_lowlevel.h>
#include "hsi_nfs3.h"
#define NFS3_COOKIEVERFSIZE 8
#define RPCCOUNT 8

#define min(x, y) ((x) < (y) ? (x) : (y))

extern struct hsfs_inode g_inode;
extern struct hsfs_super g_super;

void hsx_fuse_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                         struct fuse_file_info *fi)
{
	struct hsfs_readdir_ctx *hrc = NULL;
	struct hsfs_readdir_ctx *temp_ctx = NULL;
	struct hsfs_inode *hi = NULL;
	struct hsfs_super *hs = NULL;
	size_t *dircount = NULL;
	size_t maxcount = 0;
	size_t newlen = 0;
	char * buf = NULL;
	int err=0;

	buf = (char *) malloc(size);
	if( NULL == buf){
		err = ENOMEM;
		fuse_reply_err(req, err);
		goto out;
	}

	hrc = (struct hsfs_readdir_ctx*)malloc(sizeof(struct hsfs_readdir_ctx));
	hs = fuse_req_userdata(req);
	memset(hrc, 0, sizeof(struct hsfs_readdir_ctx));
	maxcount = RPCCOUNT*size;
	hrc->off = off;
	hi = hsx_fuse_iget(hs,ino);

	err = hsi_nfs3_readdir(hi, hrc, dircount, maxcount);
	
	if(err)
	{
		fuse_reply_err(req, err);
		goto out;
	}

	temp_ctx = hrc;
	hrc = hrc->next;
	while(hrc!=NULL){
	  	size_t tmplen = newlen;
	  	newlen += fuse_add_direntry(req, buf + tmplen, size -tmplen, 
				hrc->name, &hrc->stbuf, hrc->off);
	  	if(newlen>size)
	    		break;
		  
		hrc = hrc->next;
		
	}

       	size_t temp_size = min(newlen, size);
	if (newlen)
		fuse_reply_buf(req, buf, temp_size);
	else
		fuse_reply_buf(req, NULL, 0);
	

out:	
	if(NULL != buf)
		free(buf);
	while(temp_ctx->next != NULL){
		hrc = temp_ctx->next;
		free(temp_ctx);
		temp_ctx = hrc;
	}	
	free(temp_ctx);
		
	return;
}


