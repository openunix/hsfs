/*hsx_fuse_write.c*/


#include <fuse_lowlevel.h>
#include "hsi_nfs3.h"

extern struct hsfs_inode g_inode;
extern struct hsfs_super g_super;

extern void hsx_fuse_reply_err(fuse_req_t req,int stat);

void hsx_fuse_write(fuse_req_t req, fuse_ino_t ino, const char *buf,
                          size_t size, off_t off, struct fuse_file_info *fi)
{
	struct hsfs_rw_info winfo;
	struct hsfs_super * sb = &g_super;//(struct hsfs_super *)fuse_req_userdata(req);
	size_t cnt = 0;
	size_t err = 12;//NFS3ERR_NOMEM
	char * buffer = NULL;
	
	buffer = (char *) malloc(sb->wtmax);
	if( NULL == buffer)
		hsx_fuse_replay_err(req, err);
	
	memset(&winfo, 0, sizeof(struct hsfs_rw_info));
	if(fi->direct_io)
		winfo.stable = FUSE_FILE_SYNC;
	else
		winfo.stable = FUSE_UNSTABLE;
	winfo.inode = &g_inode;//hsx_fuse_iget(ino);
	while(cnt < size)
	{
		size_t tmp_size = min(size - cnt, sb->wtmax);
		
		winfo.rw_size = tmp_size;
		winfo.rw_off = off + cnt;
		memcpy(buffer, buf + cnt, min(size, sb->wtmax));
		winfo.data.data_len = tmp_size;
		winfo.data.data_val = buffer;
		err = hsi_nfs3_write(&winfo);
		
		if(err)
		{
			hsx_fuse_replay_err(req, err);
			goto out;
		}
			
		cnt += winfo.ret_count;

		if(winfo.eof)
			break;
	}

	fuse_reply_write(req, cnt);
out:	
	if(NULL != buffer)
		free(buffer);
		
	return;

}

