/*hsx_fuse_write.c*/

#include <errno.h>
#include "hsi_nfs3.h"

void hsx_fuse_write(fuse_req_t req, fuse_ino_t ino, const char *buf,
                          size_t size, off_t off, struct fuse_file_info *fi)
{
	struct hsfs_rw_info winfo;
	struct hsfs_super * sb = (struct hsfs_super *)fuse_req_userdata(req);
	size_t cnt = 0;
	int err = 0;
	char * buffer = NULL;
	
	DEBUG_IN("ino %d offset 0x%x size 0x%x", (unsigned int)ino, (unsigned int)off, (unsigned int)size);
	buffer = (char *) malloc(sb->wsize);
	if( NULL == buffer){
		err = ENOMEM;
		fuse_reply_err(req, err);
		goto out;
	}
	
	memset(&winfo, 0, sizeof(struct hsfs_rw_info));
	if(fi->direct_io)
		winfo.stable = HSFS_FILE_SYNC;
	else
		winfo.stable = HSFS_UNSTABLE;
	if(NULL == (winfo.inode = hsx_fuse_iget(sb, ino))){
		err = ENOENT;
		fuse_reply_err(req, err);
		goto out;
	}
	while(cnt < size){
		size_t tmp_size = min(size - cnt, sb->wsize);
		
		winfo.rw_size = tmp_size;
		winfo.rw_off = off + cnt;
		memcpy(buffer, buf + cnt, tmp_size);
		winfo.data.data_len = tmp_size;
		winfo.data.data_val = buffer;
		err = hsi_nfs3_write(&winfo);
		
		if(err){
			fuse_reply_err(req, err);
			goto out;
		}
			
		cnt += winfo.ret_count;
	}

	fuse_reply_write(req, cnt);
out:	
	if(NULL != buffer)
		free(buffer);
	
	DEBUG_OUT("err %d", err);		
	return;

}

