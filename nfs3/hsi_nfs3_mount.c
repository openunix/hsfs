
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mntent.h>
#include <errno.h>
#include <rpc/rpc.h>
#include <rpc/rpc_com.h>

#include "log.h"
#include "hsfs.h"
#include "conn.h"
#include "nfs3.h"
#include "hsi_nfs3.h"
#include "mount.h"
#include "xcommon.h"
#include "fstab.h"

typedef dirpath mntarg_t;
typedef dirpath umntarg_t;
typedef struct mountres3 mntres_t;

#define NFS_MOUNT_TCP		0x0001

static clnt_addr_t nfs_server_bak = {};
static int ssize_bak = 0;
static int rsize_bak = 0;

static int hsi_gethostbyname(const char *hostname, struct sockaddr_in *saddr)
{
	struct hostent *hp = NULL;

	saddr->sin_family = AF_INET;
	if (!inet_aton(hostname, &saddr->sin_addr)) {
		if ((hp = gethostbyname(hostname)) == NULL) {
			ERR("mount: can't get address for %s.", hostname);
			return errno;
		} else {
			if (hp->h_length > sizeof(*saddr)) {
				ERR("mount: got bad hp->h_length.");
				hp->h_length = sizeof(*saddr);
			}
			memcpy(&saddr->sin_addr, hp->h_addr, hp->h_length);
		}
	}
	return 0;
}

static int hsi_nfsmnt_check_compat(const struct pmap *nfs_pmap,
				const struct pmap *mnt_pmap)
{
	if (nfs_pmap->pm_vers) {
		if (nfs_pmap->pm_vers != 3) {
			ERR("%s: NFS version %ld is not supported, only 3.",
					progname, nfs_pmap->pm_vers);
			goto out_bad;
		}
	}

	if (mnt_pmap->pm_vers) {
		if (mnt_pmap->pm_vers != 3)
			ERR("%s: NFS mount version %ld is not supported, only 3.",
				progname, mnt_pmap->pm_vers);
		goto out_bad;
	}

	return 0;
out_bad:
	return 1;
}

static CLIENT *hsi_mnt_openclnt(clnt_addr_t *mnt_server,
					const unsigned long sb_size,
					const unsigned long rb_size)
{
	struct sockaddr_in *maddr = &mnt_server->saddr;
	struct pmap *mp = &mnt_server->pmap;
	CLIENT *clnt = NULL;
	int fd = RPC_ANYSOCK, ret = 0;

	maddr->sin_port = htons((u_short)mp->pm_port);

	switch (mp->pm_prot) {
	case IPPROTO_UDP:
		clnt = clntudp_bufcreate(maddr, mp->pm_prog,	mp->pm_vers,
					 RETRY_TIMEOUT, &fd,
					 sb_size, rb_size);
		break;
	case IPPROTO_TCP:
		clnt = clnttcp_create(maddr, mp->pm_prog, mp->pm_vers,
				      &fd, sb_size, rb_size);
		break;
	default:
		ERR("Not supported protocol: %lu.", mp->pm_prot);
		goto out;
		break;
	}

	if (!clnt) {
		ret = rpc_createerr.cf_error.re_errno;
		ERR("Create client failed: %d.", ret);
		goto out;
	}

	/* try to mount hostname:dirname */
	clnt->cl_auth = authunix_create_default();
	if (!clnt->cl_auth) {
		ret = rpc_createerr.cf_error.re_errno;
		ERR("Create authunix failed: %d.", ret);
		CLNT_DESTROY(clnt);
		clnt = NULL;
	}
out:
	return clnt;
}

static void hsi_mnt_closeclnt(CLIENT *clnt)
{
	AUTH_DESTROY(clnt->cl_auth);
	CLNT_DESTROY(clnt);
}

static int hsi_nfs3_mount(clnt_addr_t *mnt_server,
				mntarg_t *mntarg, mntres_t *mntres)
{
	struct pmap *msp = &mnt_server->pmap;
	mntres_t tres = {};
	char *tmp = NULL;
	fhandle3 *fh = NULL; 
	CLIENT *clnt = NULL;
	int ret = 0;

	DEBUG_IN("mount dir(%s) from (%s:%d) with (%lu, %lu, %lu, %lu).",
		*mntarg, *(mnt_server->hostname), mnt_server->saddr.sin_port,
		msp->pm_prog, msp->pm_vers, msp->pm_prot, msp->pm_port);

	clnt = hsi_mnt_openclnt(mnt_server, MNT_SENDBUFSIZE, MNT_RECVBUFSIZE);
	if (!clnt) {
		ret = -1;
		goto out;
	}

	ret = clnt_call(clnt, MOUNTPROC3_MNT,
			 (xdrproc_t) xdr_dirpath, (caddr_t) mntarg,
			 (xdrproc_t) xdr_mountres3, (caddr_t) &tres,
			 TIMEOUT);
	if (ret != RPC_SUCCESS) {
		ERR("Get root fh failed: %d.", ret);
	}

	/* copy filehandle to mntrest */
	fh = &(tres.mountres3_u.mountinfo.fhandle);
	tmp = malloc(fh->fhandle3_len);

	if (tmp == NULL) {
		ret = ENOMEM;
	} else {
		memcpy(tmp , fh->fhandle3_val, fh->fhandle3_len);
		*mntres = tres;
		mntres->mountres3_u.mountinfo.fhandle.fhandle3_val = tmp;
	}

	clnt_freeres(clnt, (xdrproc_t) xdr_mountres3, (caddr_t) &tres);

	hsi_mnt_closeclnt(clnt);

	DEBUG_OUT("%s", ".");
out:
	return ret;
}

static int hsi_nfs3_unmount(clnt_addr_t *mnt_server, umntarg_t *umntarg)
{
	struct pmap *msp = &mnt_server->pmap;
	CLIENT *clnt = NULL;
	int ret = 0;

	DEBUG_IN("umount dir(%s) from (%s:%d) with (%lu, %lu, %lu, %lu).",
		*umntarg, *(mnt_server->hostname), mnt_server->saddr.sin_port,
		msp->pm_prog, msp->pm_vers, msp->pm_prot, msp->pm_port);

	clnt = hsi_mnt_openclnt(mnt_server, MNT_SENDBUFSIZE, MNT_RECVBUFSIZE);
	if (!clnt) {
		ret = -1;
		goto out;
	}

	ret = clnt_call(clnt, MOUNTPROC_UMNT,
			(xdrproc_t)xdr_dirpath, (caddr_t)umntarg,
			(xdrproc_t)xdr_void, NULL,
			TIMEOUT);
	if (ret != RPC_SUCCESS) {
		ERR("Get root fh failed: %d.", ret);
	}

	hsi_mnt_closeclnt(clnt);

	DEBUG_OUT("%s", ".");
out:
	return ret;
}

static CLIENT *hsi_nfs3_clnt_create(clnt_addr_t *nfs_server,
						int ssize, int rsize)
{
	CLIENT *clnt = NULL;
	static char clnt_res;
	int ret = 0;

	clnt = hsi_mnt_openclnt(nfs_server, ssize, rsize);
	if (!clnt)
		goto out;

	memset(&clnt_res, 0, sizeof(clnt_res));
	ret = clnt_call(clnt, NULLPROC,
			 (xdrproc_t)xdr_void, (caddr_t)NULL,
			 (xdrproc_t)xdr_void, (caddr_t)&clnt_res,
			 TIMEOUT);
	if (ret) {
		ERR("Ping remote NFSv3 failed: %d.", ret);
		hsi_mnt_closeclnt(clnt);
		clnt = NULL;
		goto out;
	};

	ssize_bak = ssize;
	rsize_bak = rsize;
	nfs_server_bak = *nfs_server;
out:
	return clnt;
}

static int hsi_parse_spec(char *devname, char **hostname,
				char **pathname)
{
	char *s = NULL;

	if (devname == NULL)
		return EINVAL;

	if ((s = strchr(devname, ':'))) {
		*hostname = devname;
		*pathname = s + 1;
		*s = '\0';
		/* Ignore all but first hostname in replicated mounts
		   until they can be fully supported. (mack@sgi.com) */
		if ((s = strchr(devname, ','))) {
			*s = '\0';
			ERR("%s: warning: multiple hostnames not supported.",
				progname);
		}
	} else {
		ERR("%s: directory to mount not in host:dir format.", progname);
		return -1;
	}

	return 0;
}

static int hsi_nfs3_parse_options(char *old_opts, struct hsfs_super *super,
				  int *retry, clnt_addr_t *mnt_server,
				  clnt_addr_t *nfs_server,
				  char *new_opts, const int opt_size)
{
	struct sockaddr_in *mnt_saddr = &mnt_server->saddr;
	struct pmap *mnt_pmap = &mnt_server->pmap;
	struct pmap *nfs_pmap = &nfs_server->pmap;
	char *opt, *opteq, *p, *opt_b, *tmp_opts;
	int open_quote = 0, len = 0;
	char *mounthost = NULL;
	char cbuf[128];

	len = strlen(new_opts);
	tmp_opts = xstrdup(old_opts);
	for (p=tmp_opts, opt_b=NULL; p && *p; p++) {
		if (!opt_b)
			opt_b = p;		/* begin of the option item */
		if (*p == '"')
			open_quote ^= 1;	/* reverse the status */
		if (open_quote)
			continue;		/* still in a quoted block */
		if (*p == ',')
			*p = '\0';		/* terminate the option item */
		if (*p == '\0' || *(p+1) == '\0') {
			opt = opt_b;		/* opt is useful now */
			opt_b = NULL;
		}
		else
			continue;		/* still somewhere in the option item */

		if (strlen(opt) >= sizeof(cbuf))
			goto bad_parameter;
		if ((opteq = strchr(opt, '=')) && isdigit(opteq[1])) {
			int val = atoi(opteq + 1);	
			*opteq = '\0';
			if (!strcmp(opt, "rsize"))
				super->rsize = val;
			else if (!strcmp(opt, "wsize"))
				super->wsize = val;
			else if (!strcmp(opt, "timeo"))
				super->timeo = val;
			else if (!strcmp(opt, "retrans"))
				super->retrans = val;
			else if (!strcmp(opt, "acregmin"))
				super->acregmin = val;
			else if (!strcmp(opt, "acregmax"))
				super->acregmax = val;
			else if (!strcmp(opt, "acdirmin"))
				super->acdirmin = val;
			else if (!strcmp(opt, "acdirmax"))
				super->acdirmax = val;
			else if (!strcmp(opt, "actimeo")) {
				super->acregmin = val;
				super->acregmax = val;
				super->acdirmin = val;
				super->acdirmax = val;
			}
			else if (!strcmp(opt, "retry"))
				*retry = val;
			else if (!strcmp(opt, "port"))
				nfs_pmap->pm_port = val;
			else if (!strcmp(opt, "mountport"))
			        mnt_pmap->pm_port = val;
			else if (!strcmp(opt, "mountprog"))
				mnt_pmap->pm_prog = val;
			else if (!strcmp(opt, "mountvers"))
				mnt_pmap->pm_vers = val;
			else if (!strcmp(opt, "mounthost"))
				mounthost=xstrndup(opteq+1, strcspn(opteq+1," \t\n\r,"));
			else if (!strcmp(opt, "nfsprog"))
				nfs_pmap->pm_prog = val;
			else if (!strcmp(opt, "nfsvers") ||
				 !strcmp(opt, "vers")) {
				nfs_pmap->pm_vers = val;
				opt = "nfsvers";
			} else if (!strcmp(opt, "namlen")) {
				continue;
			} else if (!strcmp(opt, "addr")) {
				/* ignore */;
				continue;
 			} else
				goto bad_parameter;
			sprintf(cbuf, "%s=%s,", opt, opteq+1);
		} else if (opteq) {
			*opteq = '\0';
			if (!strcmp(opt, "proto")) {
				if (!strcmp(opteq+1, "udp")) {
					nfs_pmap->pm_prot = IPPROTO_UDP;
					mnt_pmap->pm_prot = IPPROTO_UDP;
					super->flags &= ~NFS_MOUNT_TCP;
				} else if (!strcmp(opteq+1, "tcp")) {
					nfs_pmap->pm_prot = IPPROTO_TCP;
					mnt_pmap->pm_prot = IPPROTO_TCP;
					super->flags |= NFS_MOUNT_TCP;
				} else
					goto bad_parameter;
			} else if (!strcmp(opt, "sec")) {
				char *secflavor = opteq+1;
				WARNING("Warning: ignoring sec=%s option", secflavor);
					continue;
			} else if (!strcmp(opt, "mounthost"))
				mounthost=xstrndup(opteq+1,
						   strcspn(opteq+1," \t\n\r,"));
			 else if (!strcmp(opt, "context")) {
				WARNING("Warning: ignoring context option");
			 } else
				goto bad_parameter;
			sprintf(cbuf, "%s=%s,", opt, opteq+1);
		} else {
			int val = 1;
			if (!strncmp(opt, "no", 2)) {
				val = 0;
				opt += 2;
			}
			if (!strcmp(opt, "bg"))
				continue;
			else if (!strcmp(opt, "fg"))
				continue;
			else if (!strcmp(opt, "soft")) {
				continue;
			} else if (!strcmp(opt, "hard")) {
				continue;
			} else if (!strcmp(opt, "intr")) {
				continue;
			} else if (!strcmp(opt, "posix")) {
				continue;
			} else if (!strcmp(opt, "cto")) {
				continue;
			} else if (!strcmp(opt, "ac")) {
				continue;
			} else if (!strcmp(opt, "tcp")) {
				if (val) {
					nfs_pmap->pm_prot = IPPROTO_TCP;
					mnt_pmap->pm_prot = IPPROTO_TCP;
					super->flags |= NFS_MOUNT_TCP;
				} else {
					mnt_pmap->pm_prot = IPPROTO_UDP;
					nfs_pmap->pm_prot = IPPROTO_UDP;
				}
			} else if (!strcmp(opt, "udp")) {
				super->flags &= ~NFS_MOUNT_TCP;
				if (!val) {
					nfs_pmap->pm_prot = IPPROTO_TCP;
					mnt_pmap->pm_prot = IPPROTO_TCP;
					super->flags |= NFS_MOUNT_TCP;
				} else {
					nfs_pmap->pm_prot = IPPROTO_UDP;
					mnt_pmap->pm_prot = IPPROTO_UDP;
				}
			} else if (!strcmp(opt, "lock")) {
				continue;
			} else if (!strcmp(opt, "broken_suid")) {
				continue;
			} else if (!strcmp(opt, "acl")) {
				continue;
			} else if (!strcmp(opt, "rdirplus")) {
				continue;
			} else if (!strcmp(opt, "sharecache")) {
				continue;
			} else {
				WARNING("%s: Unsupported nfs mount option:"
						" %s%s", progname,
						val ? "" : "no", opt);
				goto out_bad;
			}
			sprintf(cbuf, val ? "%s," : "no%s,", opt);
		}
		len += strlen(cbuf);
		if (len >= opt_size) {
			ERR("%s: excessively long option argument", progname);
			goto out_bad;
		}
		strcat(new_opts, cbuf);
	}
	/* See if the nfs host = mount host. */
	if (mounthost) {
		if (hsi_gethostbyname(mounthost, mnt_saddr) != 0)
			goto out_bad;
		*mnt_server->hostname = mounthost;
	}
	free(tmp_opts);
	return 0;
 bad_parameter:
	ERR("%s: Bad nfs mount parameter: %s\n", progname, opt);
	return 0;
 out_bad:
	free(tmp_opts);
	return 1;
}

/*
 * Determine the actual block size (and log2 thereof)
 */
unsigned long hsfs_block_bits(unsigned long bsize, unsigned char *nrbitsp)
{
	/* make sure blocksize is a power of two */
	if ((bsize & (bsize - 1)) || nrbitsp) {
		unsigned char	nrbits;

		for (nrbits = 31; nrbits && !(bsize & (1 << nrbits)); nrbits--)
			;
		bsize = 1 << nrbits;
		if (nrbitsp)
			*nrbitsp = nrbits;
	}

	return bsize;
}

/*
 * Compute and set NFS server blocksize
 */
unsigned long hsfs_block_size(unsigned long bsize, unsigned char *nrbitsp)
{
	if (bsize < HSFS_MIN_FILE_IO_SIZE)
		bsize = HSFS_DEF_FILE_IO_SIZE;
	else if (bsize >= HSFS_MAX_FILE_IO_SIZE)
		bsize = HSFS_MAX_FILE_IO_SIZE;

	return hsfs_block_bits(bsize, nrbitsp);
}

void hsi_validate_mount_data(struct hsfs_super *super, clnt_addr_t *ms,
					clnt_addr_t *ns, int *retry)
{
	struct pmap *mp = &ms->pmap;
	struct pmap *np = &ns->pmap;

	if (!super->timeo)
		super->timeo = 600;

	if (!super->retrans)
		super->retrans = 3;

	if (!super->acregmin)
		super->acregmin = 3;
	if (!super->acregmax)
		super->acregmax = 60;
	if (super->acregmin > super->acregmax) {
		super->acregmin = 3;
		super->acregmax = 60;
	}

	if (!super->acdirmin)
		super->acdirmin = 30;
	if (!super->acdirmax)
		super->acdirmax = 60;
	if (super->acdirmin > super->acdirmax) {
		super->acdirmin = 30;
		super->acdirmax = 60;
	}

	super->bsize = hsfs_block_size(super->bsize, &super->bsize_bits);

	/* init mount server */
	memcpy(&ms->saddr, &super->addr, sizeof(struct sockaddr_in));
	mp->pm_prog = MOUNTPROG;
	mp->pm_vers = MOUNTVERS_NFSV3;
	if (!mp->pm_prot)
		mp->pm_prot = IPPROTO_TCP;

	/* init nfs server */
	memcpy(&ns->saddr, &super->addr, sizeof(struct sockaddr_in));
	np->pm_prog = NFS_PROGRAM;
	np->pm_vers = NFS_V3;
	if (!np->pm_prot)
		np->pm_prot = IPPROTO_TCP;

	/* initial as __rpc_get_t_size at libtirpc */
	if (np->pm_prot == IPPROTO_TCP) {
		super->rsize = super->wsize = 64 * 1024;
	} else if (np->pm_prot == IPPROTO_UDP) {
		super->rsize = super->wsize = UDPMSGSIZE;
	} else {
		super->rsize = super->wsize = RPC_MAXDATASIZE;
	}

	/* init retry */
	if (*retry == -1) {
		*retry = 10000;  /* 10000 mins == ~1 week*/
	}
}

static int hsi_fill_super(struct hsfs_super *super, nfs_fh3 *fh)
{
	struct hsfs_inode *root = NULL;
	int ret = 0;

	ret = hsx_fuse_itable_init(super);
	if (ret)
		goto out;

	root = hsi_nfs3_alloc_node(super, fh, FUSE_ROOT_ID, NULL);
	if (root == NULL) {
		ret = ENOMEM;
		goto out;
	}

	super->root = root;
	hsx_fuse_iadd(super, root);

	ret = hsi_nfs3_fsinfo(super->root);
	if (ret)
		goto out;
	
	ret = hsi_nfs3_pathconf(super->root);
	if (ret)
		goto out;
out:
	return ret;
}

struct fuse_chan *hsx_fuse_mount(const char *spec, const char *point,
				  struct fuse_args *args, char *udata,
				  struct hsfs_super *super)
{
	struct fuse_chan *ch = NULL;
	char hostdir[1024] = {};
	char *hostname, *dirname, *old_opts;
	char new_opts[1024] = {};
	clnt_addr_t mnt_server = { 
		.hostname = &hostname 
	};
	clnt_addr_t nfs_server = { 
		.hostname = &hostname 
	};

	struct sockaddr_in *nfs_saddr = &nfs_server.saddr;
	struct pmap  *mnt_pmap = &mnt_server.pmap,
		     *nfs_pmap = &nfs_server.pmap;
	struct pmap  save_mnt, save_nfs;

	mntres_t mntres = {};
	
	int retry = 0, ret = 0;
	char *s = NULL;
	time_t timeout, t;

	if (strlen(spec) >= sizeof(hostdir)) {
		ERR("%s: excessively long host:dir argument.", progname);
		goto fail;
	}

	strcpy(hostdir, spec);
	if (hsi_parse_spec(hostdir, &hostname, &dirname))
		goto fail;

	if (hsi_gethostbyname(hostname, &super->addr) != 0)
		goto fail;

	/* add IP address to mtab options for use when unmounting */
	s = inet_ntoa(nfs_saddr->sin_addr);

	old_opts = udata;
	if (!old_opts)
		old_opts = "";

	retry = -1;
	new_opts[0] = 0;
	if (hsi_nfs3_parse_options(old_opts, super, &retry, &mnt_server,
				    &nfs_server, new_opts, sizeof(new_opts)))
		goto fail;

	if (hsi_nfsmnt_check_compat(nfs_pmap, mnt_pmap))
		goto fail;

	/* Set default options, call after hsi_nfs3_parse_options. */
	hsi_validate_mount_data(super, &mnt_server, &nfs_server, &retry);

	if (verbose) {
		INFO("rsize = %d, wsize = %d, timeo = %d, retrans = %d",
		       super->rsize, super->wsize, super->timeo, super->retrans);
		INFO("acreg (min, max) = (%d, %d), acdir (min, max) = (%d, %d)",
		       super->acregmin, super->acregmax, super->acdirmin, super->acdirmax);
		INFO("mountprog = %lu, mountvers = %lu, nfsprog = %lu, nfsvers = %lu",
		       mnt_pmap->pm_prog, mnt_pmap->pm_vers,
		       nfs_pmap->pm_prog, nfs_pmap->pm_vers);
		INFO("mountprot = %lu, mountport = %lu, nfsprot = %lu, nfsvport = %lu",
		       mnt_pmap->pm_prot, mnt_pmap->pm_port,
		       nfs_pmap->pm_prot, nfs_pmap->pm_port);
	}

	memcpy(&save_nfs, nfs_pmap, sizeof(save_nfs));
	memcpy(&save_mnt, mnt_pmap, sizeof(save_mnt));

	timeout = time(NULL) + 60 * retry;
	for (;;) {
		int val = 1;

		if (!fg) {
			sleep(val);	/* 1, 2, 4, 8, 16, 30, ... */
			val *= 2;
			if (val > 30)
				val = 30;
		}

		ret = hsi_nfs3_mount(&mnt_server, &dirname, &mntres);
		if (!ret) {
			/* success! */
			break;
		}

		memcpy(nfs_pmap, &save_nfs, sizeof(*nfs_pmap));
		memcpy(mnt_pmap, &save_mnt, sizeof(*mnt_pmap));

		if (fg) {
			switch(rpc_createerr.cf_stat){
			case RPC_TIMEDOUT:
				break;
			case RPC_SYSTEMERROR:
				if (errno == ETIMEDOUT)
					break;
			default:
				ERR("Mount failed for rpcerr: %d.",
					rpc_createerr.cf_stat);
				goto fail;
			}

			t = time(NULL);
			if (t >= timeout) {
				ERR("Mount failed for rpcerr: %d, timeout.",
					rpc_createerr.cf_stat);;
				goto fail;
			}
			ERR("Mount failed%d.", ret);
			continue;
		}

		if (fg && retry > 0)
			goto fail;
	}

	/* nfs3 client */
	super->clntp = hsi_nfs3_clnt_create(&nfs_server, super->wsize,
						super->rsize);
	if (super->clntp == NULL) {
		goto umnt_fail;
	}

	/* root filehandle */
	{
		mountres3_ok *mountres;
		fhandle3 *fhandle;
		int n_flavors, *flavor;
		if (mntres.fhs_status != 0) {
			ERR("%s: %s:%s failed, reason given by server: %d.",
					progname, hostname, dirname,
					mntres.fhs_status);
			goto fail;
		}

		mountres = &mntres.mountres3_u.mountinfo;
		n_flavors = mountres->auth_flavors.auth_flavors_len;
		flavor = mountres->auth_flavors.auth_flavors_val;
		/* skip flavors */

		fhandle = &mntres.mountres3_u.mountinfo.fhandle;
		if (hsi_fill_super(super, (nfs_fh3 *)fhandle))
			goto umnt_fail;

		free(mntres.mountres3_u.mountinfo.fhandle.fhandle3_val);
	}

	ch = fuse_mount(point, args);
	if (ch == NULL) {
		goto clnt_des;
	}

	return ch;
clnt_des:
	clnt_destroy(super->clntp);
umnt_fail:
	free(mntres.mountres3_u.mountinfo.fhandle.fhandle3_val);
	hsi_nfs3_unmount(&mnt_server, &dirname);
fail:
	return NULL;
}

int hsx_fuse_unmount(const char *spec, const char *point,
					struct fuse_chan *ch,
					struct hsfs_super *super)
{
	char hostdir[1024] = {};
	char *hostname, *dirname;
	clnt_addr_t mnt_server = { 
		.hostname = &hostname 
	};
	
	struct pmap *ump = &mnt_server.pmap;

	strcpy(hostdir, spec);
	if (hsi_parse_spec(hostdir, &hostname, &dirname))
		return -1;

	fuse_unmount(point, ch);
	CLNT_DESTROY(super->clntp);

	memcpy(&mnt_server.saddr, &super->addr, sizeof(struct sockaddr_in));
	ump->pm_prog = MOUNTPROG;
	ump->pm_vers = MOUNTVERS_NFSV3;
	if (super->flags & NFS_MOUNT_TCP) {
		ump->pm_prot = IPPROTO_TCP;
	} else {
		ump->pm_prot = IPPROTO_UDP;
	}

	hsx_fuse_idel(super, super->root);
	hsx_fuse_itable_del(super);

	return hsi_nfs3_unmount(&mnt_server, &dirname);
}


static CLIENT *hsi_nfs3_clnt_reconnect(struct hsfs_super *sb, CLIENT *clnt)
{
	if (clnt == sb->clntp) {
		hsi_mnt_closeclnt(clnt);
		clnt = hsi_nfs3_clnt_create(&nfs_server_bak,
					ssize_bak, ssize_bak);
		if (clnt) {
			sb->clntp = clnt;
		}
	}
	return NULL;
}

int hsi_nfs3_clnt_call(struct hsfs_super *sb, CLIENT *clnt,
				unsigned long procnum,
				xdrproc_t inproc, char *in,
				xdrproc_t outproc, char *out)
{
	struct timeval tout = {sb->timeo / 10, sb->timeo % 10 * 100000};
	enum clnt_stat st = RPC_SUCCESS;
	int rtry = 0, ret = 0;
retry:
	st = clnt_call(clnt, procnum, inproc, in, outproc, out, tout);
	if (st != RPC_SUCCESS) {
		ret = hsi_rpc_stat_to_errno(clnt);

		if (rtry >= 5) {
			ERR("Have retry %d times, break!", rtry);
		}else if (ret == EAGAIN) {
			sleep(1);
			rtry++;
			goto retry;
		} else if (ret == ENOTCONN || ret == ECONNRESET) {
			clnt = hsi_nfs3_clnt_reconnect(sb, clnt);
			if (clnt != NULL) {
				rtry++;
				goto retry;
			}
		}

		ERR("Sending rpc request failed: %d(%d).", st, ret);
	}

	return ret;
}
