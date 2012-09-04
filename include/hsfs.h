
#ifndef __HSFS_H__
#define __HSFS_H__ 1

#define FUSE_USE_VERSION 26
#define _BSD_SOURCE

#include <fuse_lowlevel.h>
#include <rpc/rpc.h>

#define HSFS_TYPE	"hsfs"

extern char *progname;
extern int nomtab;
extern int verbose;
extern int fg;

struct hsfs_fh {
        unsigned short          size;
        unsigned char           data[64];
};

struct hsfs_super {
	CLIENT			*clntp;
	int			flags;
	int			rsize;
	int			wsize;
	int			timeo;
	int			retrans;
	int			acregmin;
	int			acregmax;
	int			acdirmin;
	int			acdirmax;
	struct sockaddr_in	addr;
	unsigned int		bsize;
	struct hsfs_fh		root;
};

struct hsfs_rw_info
{
	struct hsfs_inode *   inode;//目标文件 
	size_t       rw_size;//读写大小
	off_t        rw_off;//读写偏移
	size_t       ret_count;//读写返回的大小
	int          eof;//文件尾标记
	int          stable;//是否缓存
	struct {
				 size_t data_len;
				 char *data_val;
			} data;//数据buffer
};

struct hsfs_sattr{
	int	valid;
	mode_t	mode;
	uid_t	uid;
	gid_t	gid;
	off_t	size;
	struct timespec	atime;
	struct timespec	mtime;
}hsfs_sattr_t;

extern struct fuse_chan * hsi_fuse_mount(const char *spec, const char *point,
					 struct fuse_args *args, char *udata,
					 struct hsfs_super *userdata);

#endif
