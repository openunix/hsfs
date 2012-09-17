/**
 * hsi_nfs3_unlink.c
 * yanhuan
 * 2012.9.6
 **/

#ifdef HSFS_NFS3_TEST
# include <rpc/rpc.h>
# include <libgen.h>
# include <errno.h>
# include <arpa/inet.h>		/* inet_ntoa */
# include "nfs3.h"
# include "../include/log.h"
# include "apis.h"

char *progname;
char *svraddr;
struct hsfs_super {
	CLIENT			*clntp;
	int			timeo;
	struct sockaddr_in	addr;
};
struct hsfs_inode
{
	uint64_t		ino;
	unsigned long 		generation;
	nfs_fh3			fh;
	unsigned long		nlookup;
	fattr3_new		attr;
	struct hsfs_super	*sb;
	struct hsfs_inode 	*next;
};
#else
#include <errno.h>
#include "hsi_nfs3.h"
#endif

static const struct {
	int nfs_err;
	int sys_err;
} nfs_errtbl[] = {
	{ NFS3_OK,              0               },
	{ NFS3ERR_PERM,         EPERM		},
	{ NFS3ERR_NOENT,	ENOENT		},
	{ NFS3ERR_IO,		EIO		},
	{ NFS3ERR_NXIO,		ENXIO          	},
	/*      { NFS3ERR_EAGAIN,       -EAGAIN         }, */
	{ NFS3ERR_ACCES,        EACCES		},
	{ NFS3ERR_EXIST,        EEXIST         	},
	{ NFS3ERR_XDEV,         EXDEV          	},
	{ NFS3ERR_NODEV,        ENODEV         	},
	{ NFS3ERR_NOTDIR,       ENOTDIR        	},
	{ NFS3ERR_ISDIR,        EISDIR         	},
	{ NFS3ERR_INVAL,        EINVAL         	},
	{ NFS3ERR_FBIG,         EFBIG          	},
	{ NFS3ERR_NOSPC,        ENOSPC         	},
	{ NFS3ERR_ROFS,         EROFS          	},
	{ NFS3ERR_MLINK,        EMLINK         	},
	{ NFS3ERR_NAMETOOLONG,  ENAMETOOLONG   	},
	{ NFS3ERR_NOTEMPTY,     ENOTEMPTY      	},
	{ NFS3ERR_DQUOT,        EDQUOT         	},
	{ NFS3ERR_STALE,        ESTALE         	},
	{ NFS3ERR_REMOTE,       EREMOTE        	},
#ifdef EWFLUSH
	{ NFS3ERR_WFLUSH,       EWFLUSH        	},
#endif
	/*
	   { NFS3ERR_BADHANDLE,  -EBADHANDLE     	},
	   { NFS3ERR_NOT_SYNC,   -ENOTSYNC       	},
	   { NFS3ERR_BAD_COOKIE, -EBADCOOKIE     	},
	   { NFS3ERR_NOTSUPP,    -ENOTSUPP       	},
	   { NFS3ERR_TOOSMALL,   -ETOOSMALL      	},
	   { NFS3ERR_SERVERFAULT,-EREMOTEIO      	},
	   { NFS3ERR_BADTYPE,    -EBADTYPE       	},
	   { NFS3ERR_JUKEBOX,    -EJUKEBOX       	},
	   */
	{ -1,                   EIO           	}
};

int hsi_nfs3_stat_to_errno(int stat)
{
	int i;
	for (i = 0; nfs_errtbl[i].nfs_err != -1; i++) {
		if (nfs_errtbl[i].nfs_err == stat)
			return nfs_errtbl[i].sys_err;
	}
	return nfs_errtbl[i].sys_err;
}

int hsi_rpc_stat_to_errno(CLIENT *clntp)
{
	struct rpc_err err;
	memset(&err, 0, sizeof(err));
	clnt_geterr(clntp, &err);
	return err.re_errno == 0 ? EIO : err.re_errno;
}

int hsi_nfs3_unlink(struct hsfs_inode *parent, const char *name)
{
	int ret = 0;
	int err = 0;
	struct hsfs_super *sb = parent->sb;
	CLIENT *clntp = sb->clntp;
	diropargs3 args;
	wccstat3 res;

	DEBUG_IN(" %s %lu", name, parent->ino);

	memset(&args, 0, sizeof(args));
	memset(&res, 0, sizeof(res));
	args.dir = parent->fh;
	args.name = (char *)name;

	err = hsi_nfs3_clnt_call(sb, clntp, NFSPROC3_REMOVE,
			(xdrproc_t)xdr_diropargs3,	(caddr_t)&args,
			(xdrproc_t)xdr_wccstat3, (caddr_t)&res);
	if (err)
		goto out1;

	ret = res.status;
	if (NFS3_OK != ret) {
		ERR("Call NFS3 Server failure:(%d).\n", ret);
		err = hsi_nfs3_stat_to_errno(ret);
		goto out2;
	}
	if (res.wccstat3_u.wcc.after.present) {
		memcpy(&(parent->attr), &res.wccstat3_u.wcc.
				after.post_op_attr_u.attributes,
				sizeof(fattr3));
	}
out2:
	clnt_freeres(clntp, (xdrproc_t)xdr_wccstat3, (char *)&res);
out1:
	DEBUG_OUT(" %s %lu errno:%d", name, parent->ino, err);
	return err;
}

#ifdef HSFS_NFS3_TEST
int main(int argc, char *argv[])
{
	int err = 0;
	struct hsfs_super sb;
	progname = basename(argv[0]);
	sb.addr.sin_addr.s_addr = inet_addr("10.10.211.154");
	svraddr = inet_ntoa(*(struct in_addr *)&sb.addr.sin_addr);
	sb.clntp = clnt_create(svraddr,	NFS_PROGRAM, NFS_V3, "udp");
	if (NULL == sb.clntp) {
		fprintf(stderr, "%s: Create handle to RPC server "
				"(%s, %u, %u) failed: (%s).\n", progname,
				svraddr, NFS_PROGRAM, NFS_V3,
				clnt_spcreateerror(progname));
		err = ENXIO;
		goto out;
	}
	char *fpath = "/rpcdir/";
	unsigned char *nfs3fhp = NULL;
	size_t fhlen;
	if ((err = map_path_to_nfs3fh(svraddr, fpath, &fhlen, &nfs3fhp))){
		fprintf(stderr, "%s: Mapping %s on server to its FH feiled.%d\n",
				progname, fpath, err);
		goto out;
	}
	struct hsfs_inode *hi = NULL;
	if (!(hi = (struct hsfs_inode *)malloc(sizeof(struct hsfs_inode)))) {
		fprintf(stderr, "%s: No enough core\n", progname);
	}
	sb.timeo = 10;
	hi->sb = &sb;
	hi->fh.data.data_len = fhlen;
	hi->fh.data.data_val = nfs3fhp;

	hsi_nfs3_unlink(hi, argv[1]);
out:
	exit(err);
}
#endif


