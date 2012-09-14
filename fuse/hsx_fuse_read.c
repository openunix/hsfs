/*hsx_fuse_read.c*/

#include <errno.h>
#include "hsi_nfs3.h"

void hsx_fuse_read (fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                         struct fuse_file_info *fi)
{
	struct hsfs_rw_info rinfo;
	struct hsfs_super * sb = (struct hsfs_super *)fuse_req_userdata(req);
	size_t cnt = 0;
	int err = 0;
	char * buf = NULL;
	
	DEBUG_IN("offset 0x%x size 0x%x", (unsigned int)off, (unsigned int)size);
	buf = (char *) malloc(size);
	if( NULL == buf){
		err = ENOMEM;
		fuse_reply_err(req, err);
		goto out;
	}
	
	memset(&rinfo, 0, sizeof(struct hsfs_rw_info));
	if(NULL == (rinfo.inode = hsx_fuse_iget(sb, ino))){
		err = ENOENT;
		fuse_reply_err(req, err);
		goto out;
	}
	DEBUG("ino %lu", rinfo.inode->ino);
	while(cnt < size){
		size_t tmp_size = min(size - cnt, sb->rsize);
		
		rinfo.rw_size = tmp_size;
		rinfo.rw_off  = off + cnt;
		rinfo.data.data_val = buf + cnt;
		rinfo.data.data_len = tmp_size;
		err = hsi_nfs3_read(&rinfo);
		
		if(err){
			fuse_reply_err(req, err);
			goto out;
		}
			
		cnt += rinfo.ret_count;

		if(rinfo.eof)
			break;
	}

	fuse_reply_buf(req, buf, cnt);
out:	
	if(NULL != buf)
		free(buf);
		
	DEBUG_OUT("err %d", err);
	return;
}
