
#ifndef __HSFS_H__
#define __HSFS_H__ 1

#define FUSE_USE_VERSION 26
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

extern struct fuse_chan * hsi_fuse_mount(const char *spec, const char *point,
					 struct fuse_args *args, char *udata,
					 struct hsfs_super *userdata);

#endif
