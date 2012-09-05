
#ifndef __HSFS_H__
#define __HSFS_H__ 1

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif

#include <sys/stat.h>
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

/*
 * Maximum number of pages that readdir can use for creating
 * a vmapped array of pages.
 */
#define HSFS_MAX_READDIR_PAGES 8

struct hsfs_fh {
        unsigned short          size;
        unsigned char           data[64];
};

struct hsfs_inode
{
        uint64_t		ino;
        unsigned long 		generation;
        nfs_fh3			fh;
        unsigned long		nlookup;
        fattr3			attr;
        struct hsfs_super	*sb;
        struct hsfs_inode 	*next;
};

struct  hsfs_table
{
        struct hsfs_inode	**array;
        size_t			use;
        size_t			size;
};

struct hsfs_super {
	CLIENT			*clntp;
	int			flags;
	/* rsize/wrise, filled up at hsx_fuse_init 
	  * for read/write
	  */
	int			rsize;
	int			wsize;
	/* For all clnt_call timeout,
	  * as deciseconds (tenths of a second)
	  */
	int			timeo;
	int			retrans;
	int			acregmin;
	int			acregmax;
	int			acdirmin;
	int			acdirmax;
	struct sockaddr_in	addr;
	/* namelen, filled up at hsx_fuse_init */
	int			namlen;
	/* Readdir size */
	int			dtsize;
	unsigned int		bsize;
	unsigned char		bsize_bits;
	struct hsfs_inode	*root;
	struct hsfs_table       hsfs_fuse_ht;
	/* Copy from FSINFO result directly, be careful */
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

	/* Copy from FSSTAT result directly, be careful */
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
 };

/* table function declaration */
extern	int hsx_fuse_init(struct hsfs_super *sb);
extern	uint64_t hsfs_ino_hash(struct hsfs_super *sb, uint64_t ino);
extern	void hsx_fuse_iadd(struct hsfs_super *sb, struct hsfs_inode *hsfs_node);
extern	struct hsfs_inode *hsx_fuse_iget(struct hsfs_super *sb, uint64_t ino);
extern	int hsx_fuse_idel(struct hsfs_super *sb, struct hsfs_inode *hs_node);

#define min(x, y) ((x) < (y) ? (x) : (y))

#endif
