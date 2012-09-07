/*
 *hsx_fuse_readdir.c
 */

#include <fuse/fuse_lowlevel.h>
#include "hsi_nfs3.h"
#define NFS3_COOKIEVERFSIZE 8

#define min(x, y) ((x) < (y) ? (x) : (y))

extern struct hsfs_inode g_inode;
extern struct hsfs_super g_super;

void hsx_fuse_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                         struct fuse_file_info *fi)
{
	struct hsfs_readdir_ctx *hrc=NULL;
	struct hsfs_inode *hi=NULL;
	struct hsfs_super *hs=NULL;
	size_t *dircount=NULL;
	size_t maxcount=0;
	size_t newlen= 0;
	char * buf = NULL;
	int err=0;

	buf = (char *) malloc(size);
	if( NULL == buf)
	  fuse_reply_err(req, err);
	
	hi = (struct hsfs_inode*)malloc(sizeof(struct hsfs_inode));
	hrc = (struct hsfs_readdir_ctx*)malloc(sizeof(struct hsfs_readdir_ctx));
	hi->sb = (struct hsfs_super *)malloc(sizeof(struct hsfs_super));
	hs = (struct hsfs_super)malloc(sizeof(struct hsfs_super));
	memset(hrc, 0, sizeof(struct hsfs_readdir_ctx));
	maxcount=size;
	hi = hsx_fuse_iget(hs,ino);
	hi->sb->clntp=clntp;
	if(off){
	  	while(off != fi->fh.off)
		  fi.fh = fi->fh.next;
	  	hrc=fi->fh;
	}

	err=hsi_nfs3_readdir(hi,hrc,dircount,maxcount);
	
	if(err)
	{
		fuse_reply__err(req, err);
		goto out;
	}
	
	hrc=hrc->next;
	while(hrc!=NULL){
	  	fi->fh=(size_t)hrc;
	  	size_t tmplen=newlen;
	  	newlen += fuse_add_direntry(req, NULL, 0, hrc->name, NULL, 0);
	 	fuse_add_direntry(req, buf + tmplen,size - tmplen, name,
			    hrc->stbuf, hrc->off);
	  	if(newlen>size)
	    		break;
	  
		hrc=hrc->next;
	}

       	size_t temp_size = min(newlen, size);
	if (newlen)
		fuse_reply_buf(req, buf, temp_size);
	else
		fuse_reply_buf(req, NULL, 0);

out:	
	if(NULL != buf)
		free(buf);
		
	return;
}


