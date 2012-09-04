/*hsx_fuse_read.c*/


#include <fuse/fuse_lowlevel.h>
#include "hsi_nfs3.h"

#define FUSE_FILE_SYNC  2
#define FUSE_UNSTABLE	0

#define min(x, y) ((x) < (y) ? (x) : (y))

extern struct hsfs_inode g_inode;
extern struct hsfs_super g_super;

void hsx_fuse_replay_err(fuse_req_t req,int stat)
{
	int err = 12;//NFS3ERR_NOMEM hsi_nfs3_stat_to_errno(stat);
	
	fuse_reply_err(req, err);
}

void hsx_fuse_read (fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                         struct fuse_file_info *fi)
{
	struct hsfs_rw_info rinfo;
	struct hsfs_super * sb = &g_super;//(struct hsfs_super *)fuse_req_userdata(req);
	size_t cnt = 0;
	size_t err = 12;//NFS3ERR_NOMEM
	char * buf = NULL;
	
	buf = (char *) malloc(size);//for fuse_reply_buf think
	if( NULL == buf)
		hsx_fuse_replay_err(req, err);
	
	memset(&rinfo, 0, sizeof(struct hsfs_rw_info));
	rinfo.inode = &g_inode;//hsx_fuse_iget(ino);
	while(cnt < size)
	{
		size_t tmp_size = min(size - cnt, sb->rtmax);
		
		rinfo.rw_size = tmp_size;
		rinfo.rw_off  = off + cnt;
		err = hsi_nfs3_read(&rinfo);
		
		if(err)
		{
			hsx_fuse_replay_err(req, err);
			goto out;
		}
			
		memcpy(buf + cnt, rinfo.data.data_val, rinfo.ret_count);
		cnt += rinfo.ret_count;

		if(rinfo.eof)
			break;
	}

	fuse_reply_buf(req, buf, cnt);
out:	
	if(NULL != buf)
		free(buf);
		
	return;
}
