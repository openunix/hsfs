
#ifndef __HSFS_H__
#define __HSFS_H__ 1

#define FUSE_USE_VERSION 26
#include <fuse/fuse_lowlevel.h>
#include <rpc/rpc.h>

#include "nfs3.h"

#define HSFS_TYPE	"hsfs"

extern char *progname;
extern int nomtab;
extern int verbose;
extern int fg;

/*
 * To change the maximum rsize and wsize supported by the NFS client, adjust
 * NFS_MAX_FILE_IO_SIZE.  64KB is a typical maximum, but some servers can
 * support a megabyte or more.  The default is left at 4096 bytes, which is
 * reasonable for NFS over UDP.
 */
#define HSFS_MAX_FILE_IO_SIZE	(1048576U)
#define HSFS_DEF_FILE_IO_SIZE	(4096U)
#define HSFS_MIN_FILE_IO_SIZE	(1024U)

struct hsfs_fh {
        unsigned short          size;
        unsigned char           data[64];
};

struct hsfs_super {
	CLIENT			*clntp;
	int			flags;
	/* rsize/wrise, filled up at hsx_fuse_init */
	int			rsize;
	int			wsize;
	int			timeo;
	int			retrans;
	int			acregmin;
	int			acregmax;
	int			acdirmin;
	int			acdirmax;
	struct sockaddr_in	addr;
	/* namelen, filled up at hsx_fuse_init */
	int			namlen;
	unsigned int		bsize;
	unsigned char		bsize_bits;
	struct hsfs_inode	*root;

	/* From FSINFO */
	uint32_t		rtmax;
	uint32_t		rtpref;
	uint32_t		rtmult;
	uint32_t		wtmax;
	uint32_t		wtpref;
	uint32_t		wtmult;
	uint32_t		dtpref;
	uint64_t		maxfilesize;
	nfstime3		time_delta;
	uint32_t		properties;

	/* From FSSTAT */
	uint64_t		tbytes;
	uint64_t		fbytes;
	uint64_t		abytes;
	uint64_t		tfiles;
	uint64_t		ffiles;
	uint64_t		afiles;
};

#define HSFS_FILE_SYNC  2
#define HSFS_UNSTABLE	0

struct hsfs_rw_info {
	struct hsfs_inode	*inode;
	size_t			rw_size;//read or write size
	off_t			rw_off;//read or write offset
	size_t			ret_count;//result of operate
	int			eof;//end of file
	int			stable;//direct io flag
	struct {
				 size_t data_len;
				 char *data_val;
			} data;//data buffer
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

struct hsfs_readdir_ctx{
	off_t	off;
	char	*name;
	char	cookieverf[NFS3_COOKIEVERFSIZE];
	struct stat	stbuf;
	struct hsfs_readdir_ctx *next;
 }

#define min(x, y) ((x) < (y) ? (x) : (y))

extern struct fuse_chan * hsi_fuse_mount(const char *spec, const char *point,
					 struct fuse_args *args, char *udata,
					 struct hsfs_super *userdata);
extern int hsi_fuse_unmount(const char *spec, const char *point,
					struct fuse_chan *ch,
					struct hsfs_super *super);
#endif
