
#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE 
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <libgen.h>
#include "hsfs.h"
#include "nfs3.h"
#include "log.h"

#ifndef errno_NFSERR_IO
#define errno_NFSERR_IO EIO
#endif

#ifndef ERESTATRSYS 
#define ERESTARTSYS     512
#endif

#ifndef ERESTARTNOINTR
#define ERESTARTNOINTR  513
#endif

#ifndef ERESTARTNOHAND
#define ERESTARTNOHAND  514     /* restart if no handler.. */
#endif 

#ifndef ENOIOCTLCMD
#define ENOIOCTLCMD     515     /* No ioctl command */
#endif

#ifndef ERESTART_RESTARTBLOCK
#define ERESTART_RESTARTBLOCK 516 /* restart by calling sys_restart_syscall */
#endif

/* Defined for the NFSv3 protocol */
#ifndef EBADHANDLE
#define EBADHANDLE      521     /* Illegal NFS file handle */
#endif

#ifndef ENOTSYNC
#define ENOTSYNC        522     /* Update synchronization mismatch */
#endif

#ifndef EBADCOOKIE
#define EBADCOOKIE      523     /* Cookie is stale */
#endif

#ifndef ENOTSUPP
#define ENOTSUPP        524     /* Operation is not supported */
#endif

#ifndef ETOOSMALL
#define ETOOSMALL       525     /* Buffer or request is too small */
#endif

#ifndef ESERVERFAULT
#define ESERVERFAULT    526     /* An untranslatable error occurred */
#endif

#ifndef EBADTYPE
#define EBADTYPE        527     /* Type not supported by server */
#endif

#ifndef EJUKEBOX
#define EJUKEBOX        528     /* Request initiated, but will not complete before timeout */
#endif

#ifndef EIOCBQUEUED
#define EIOCBQUEUED     529     /* iocb queued, will get completion event */
#endif

#ifndef EIOCBRETRY
#define EIOCBRETRY      530     /* iocb queued, will trigger a retry */
#endif

struct nfs_error {
	int stat;
  	int err;
};
struct nfs_error nfs_errtbl[] = {
	{ NFS3_OK,               0               },
	{ NFS3ERR_PERM,         -EPERM           },
	{ NFS3ERR_NOENT,        -ENOENT          },
	{ NFS3ERR_IO,           -errno_NFSERR_IO },
	{ NFS3ERR_NXIO,         -ENXIO           },
	{ NFS3ERR_ACCES,        -EACCES          },
	{ NFS3ERR_EXIST,        -EEXIST          },
	{ NFS3ERR_XDEV,         -EXDEV           },
	{ NFS3ERR_NODEV,        -ENODEV          },
	{ NFS3ERR_NOTDIR,       -ENOTDIR         },
	{ NFS3ERR_ISDIR,        -EISDIR          },
	{ NFS3ERR_INVAL,        -EINVAL          },
	{ NFS3ERR_FBIG,         -EFBIG           },
	{ NFS3ERR_NOSPC,        -ENOSPC          },
	{ NFS3ERR_ROFS,         -EROFS           },
	{ NFS3ERR_MLINK,        -EMLINK          },
	{ NFS3ERR_NAMETOOLONG,  -ENAMETOOLONG    },
	{ NFS3ERR_NOTEMPTY,     -ENOTEMPTY       },
	{ NFS3ERR_DQUOT,        -EDQUOT          },
	{ NFS3ERR_STALE,        -ESTALE          },
	{ NFS3ERR_REMOTE,       -EREMOTE         },
#ifdef EWFLUSH
	{ NFSERR_WFLUSH,        -EWFLUSH         },
#endif
	{ NFS3ERR_BADHANDLE,    -EBADHANDLE      },
	{ NFS3ERR_NOT_SYNC,     -ENOTSYNC        },
	{ NFS3ERR_BAD_COOKIE,   -EBADCOOKIE      },
	{ NFS3ERR_NOTSUPP,      -ENOTSUPP        },
	{ NFS3ERR_TOOSMALL,     -ETOOSMALL       },
	{ NFS3ERR_SERVERFAULT,  -EREMOTEIO       },
	{ NFS3ERR_BADTYPE,      -EBADTYPE        },
	{ NFS3ERR_JUKEBOX,      -EJUKEBOX        },
	{ -1,                   -EIO             }
};

int hsi_nfs3_stat_to_errno(int status){
	int i; 
	for (i = 0; nfs_errtbl[i].stat != -1; i++) {
		if (nfs_errtbl[i].stat == status)
			return nfs_errtbl[i].err;
	}
	/* fprintf(stderr, "NFS: Unrecognized nfs status value: %u\n", status); */
	ERR("NFS: Unrecognized nfs status value: %u\n", status);
	return nfs_errtbl[i].err;
}

int hsi_rpc_stat_to_errno(CLIENT *clntp){
  	struct rpc_err err;
	memset(&err, 0, sizeof(err));
	clnt_geterr(clntp, &err);
	return err.re_errno == 0 ? EIO : err.re_errno;
}

int hsi_nfs3_getattr(struct hsfs_inode *inode){
	int err = 0, i;
	nfs_fh3 fh;
	CLIENT *clntp;
	struct timeval to = {120, 0};
	struct getattr3res res;
	enum clnt_stat st;
	
	INFO("Enter hsi_nfs3_getattr().\n");

	clntp= inode->sb->clntp;
	fh.data.data_len = inode->fh.data.data_len;
	fh.data.data_val = inode->fh.data.data_val;
	/*check and print filehandle*/
	/* fprintf(stdout, "Filehandle : Len=%lu, Content=", fh.data.data_len); */
	INFO("Filehandle : Len=%lu, Content=", fh.data.data_len);
	for (i=0; i < fh.data.data_len; i++)
		/*fprintf(stdout, "%02x", (unsigned char) (fh.data.data_val[i])); */
		INFO("%02x", (unsigned char) (fh.data.data_val[i]));
	if (fh.data.data_len > 0)
		/* fprintf(stdout, "\n"); */
		INFO("\n");
	memset(&res, 0, sizeof(res));
	st = clnt_call(clntp, 1, (xdrproc_t)xdr_nfs_fh3, (caddr_t)&fh, 
		       (xdrproc_t)xdr_getattr3res, (caddr_t)&res, to);
	if (st) {
		err = hsi_rpc_stat_to_errno(clntp);
		/* fprintf(stderr, "Call RPC Server failure: %d.\n", err); */
		ERR("Call RPC Server failure: %d.\n", err);
		goto out;
	}
	if (NFS3_OK != res.status) {
		err = -hsi_nfs3_stat_to_errno(res.status);  /* return negative number */
		/* fprintf(stderr, "RPC Server return failed status : %d.\n", err); */
		ERR("RPC Server return failed status : %d.\n", err);
		goto out;
	}
	/* fill return value to fattr3 attr in struct hsfs_inode */
	inode->attr.type = res.getattr3res_u.attributes.type;
	inode->attr.mode = res.getattr3res_u.attributes.mode;
	inode->attr.nlink = res.getattr3res_u.attributes.nlink;
	inode->attr.uid = res.getattr3res_u.attributes.uid;
	inode->attr.gid = res.getattr3res_u.attributes.gid;
	inode->attr.size = res.getattr3res_u.attributes.size;
	inode->attr.used = res.getattr3res_u.attributes.used;
	inode->attr.rdev.major = res.getattr3res_u.attributes.rdev.major;
	inode->attr.rdev.minor = res.getattr3res_u.attributes.rdev.minor;
	inode->attr.fsid = res.getattr3res_u.attributes.fsid;
	inode->attr.fileid = res.getattr3res_u.attributes.fileid;
	inode->attr.atime.seconds = res.getattr3res_u.attributes.atime.seconds;
	inode->attr.atime.nseconds = res.getattr3res_u.attributes.atime.nseconds;
	inode->attr.mtime.seconds = res.getattr3res_u.attributes.mtime.seconds;
	inode->attr.mtime.nseconds = res.getattr3res_u.attributes.mtime.nseconds;
	inode->attr.ctime.seconds = res.getattr3res_u.attributes.ctime.seconds;
	inode->attr.ctime.nseconds = res.getattr3res_u.attributes.ctime.nseconds;
 out:
	INFO("Leave hsi_nfs3_getattr() with errno %d.\n", err);
	return err;
}
