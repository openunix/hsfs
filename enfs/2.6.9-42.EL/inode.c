/*
 *  linux/fs/nfs/inode.c
 *
 *  Copyright (C) 1992  Rick Sladkey
 *
 *  nfs inode and superblock handling functions
 *
 *  Modularised by Alan Cox <Alan.Cox@linux.org>, while hacking some
 *  experimental NFS changes. Modularisation taken straight from SYS5 fs.
 *
 *  Change to nfs_read_super() to permit NFS mounts to multi-homed hosts.
 *  J.S.Peatfield@damtp.cam.ac.uk
 *
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/time.h>
#include <linux/kernel.h>
#include "nfs_fs.h"
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/sunrpc/clnt.h>
#include <linux/sunrpc/stats.h>
/* qinping mod 20060331 begin */
//#include <linux/nfs_fs.h>
//#include <linux/nfs_mount.h>
//#include <linux/nfs4_mount.h>
#include "nfs_mount.h"
#include "nfs4_mount.h"
/* qinping mod 20060331 end */
#include <linux/lockd/bind.h>
#include <linux/smp_lock.h>
#include <linux/seq_file.h>
#include <linux/mount.h>
/* qinping mod 20060331 begin */
//#include <linux/nfs_idmap.h>
#include "nfs_idmap.h"
/* qinping mod 20060331 end */
#include <linux/vfs.h>

#include <asm/system.h>
#include <asm/uaccess.h>

#include "delegation.h"
/* qinping mod 20060331 begin */
#include "kerror.h"
#include "vdmap.h"
/* qinping mod 20060331 end */

#define NFSDBG_FACILITY		NFSDBG_VFS
#define NFS_PARANOIA 1

/* Maximum number of readahead requests
 * FIXME: this should really be a sysctl so that users may tune it to suit
 *        their needs. People that do NFS over a slow network, might for
 *        instance want to reduce it to something closer to 1 for improved
 *        interactive response.
 */
#define NFS_MAX_READAHEAD	(RPC_DEF_SLOT_TABLE - 1)
/*zdz add 20061205 begin*/
#define MAXRAPAGES 120
#define MINRAPAGES 30

unsigned int enfs_ra_pages = 120;
unsigned int enfs_ra_flag = 1;

struct nfs_server * pserver = NULL;
/*zdz add 20061205 end*/

#ifdef ENFS_TEST
static atomic_t enfs_saerror = ATOMIC_INIT(0);
#endif

/* qinping mod 20060331 begin */
static LIST_HEAD(inode_attrset_list);
static int inode_attrset_list_len = 0;
static spinlock_t inode_attrset_lock = SPIN_LOCK_UNLOCKED;
volatile pid_t inode_attrset_pid = 0;
static wait_queue_head_t inode_attrset_wait;

#define ATTRSET_TIMEOUT (30*HZ)
#define ATTRSET_MINTIME HZ

void inode_attrset_insert(struct inode *);
void inode_attrset_del(struct inode *);
static void inode_attrset_exec(struct inode *, int);
static int inode_attrset_bflushd(void *);
static int inode_attrset_init(void);
static void inode_attrset_exit(void);
/* qinping mod 20060331 end */

static void nfs_invalidate_inode(struct inode *);
static int nfs_update_inode(struct inode *, struct nfs_fattr *, unsigned long);

static struct inode *nfs_alloc_inode(struct super_block *sb);
static void nfs_destroy_inode(struct inode *);
static int nfs_write_inode(struct inode *,int);
static void nfs_delete_inode(struct inode *);
#ifdef __SUSE9_SUP
static void nfs_put_super(struct super_block *);

static struct rpc_auth null_auth;
static struct rpc_cred null_cred;

static struct rpc_auth *
nul_create(struct rpc_clnt *clnt, rpc_authflavor_t flavor)
{
	atomic_inc(&null_auth.au_count);
	return &null_auth;
}

static void
nul_destroy(struct rpc_auth *auth)
{
}

/*
 * Lookup NULL creds for current process
 */
static struct rpc_cred *
nul_lookup_cred(struct rpc_auth *auth, struct auth_cred *acred, int flags)
{
	return get_rpccred(&null_cred);
}

/*
 * Destroy cred handle.
 */
static void
nul_destroy_cred(struct rpc_cred *cred)
{
}

/*
 * Match cred handle against current process
 */
static int
nul_match(struct auth_cred *acred, struct rpc_cred *cred, int taskflags)
{
	return 1;
}

/*
 * Marshal credential.
 */
static u32 *
nul_marshal(struct rpc_task *task, u32 *p)
{
	*p++ = htonl(RPC_AUTH_NULL);
	*p++ = 0;
	*p++ = htonl(RPC_AUTH_NULL);
	*p++ = 0;

	return p;
}

/*
 * Refresh credential. This is a no-op for AUTH_NULL
 */
static int
nul_refresh(struct rpc_task *task)
{
	task->tk_msg.rpc_cred->cr_flags |= RPCAUTH_CRED_UPTODATE;
	return 0;
}

static u32 *
nul_validate(struct rpc_task *task, u32 *p)
{
	rpc_authflavor_t	flavor;
	u32			size;

	flavor = ntohl(*p++);
	if (flavor != RPC_AUTH_NULL) {
		printk("RPC: bad verf flavor: %u\n", flavor);
		return NULL;
	}

	size = ntohl(*p++);
	if (size != 0) {
		printk("RPC: bad verf size: %u\n", size);
		return NULL;
	}

	return p;
}

struct rpc_authops authnull_ops = {
	.owner		= THIS_MODULE,
	.au_flavor	= RPC_AUTH_NULL,
#ifdef RPC_DEBUG
	.au_name	= "NULL",
#endif
	.create		= nul_create,
	.destroy	= nul_destroy,
//zdz	.lookup_cred	= nul_lookup_cred,
};

static
struct rpc_auth null_auth = {
	.au_cslack	= 4,
	.au_rslack	= 2,
	.au_ops		= &authnull_ops,
};

static
struct rpc_credops	null_credops = {
//zdz	.cr_name	= "AUTH_NULL",
	.crdestroy	= nul_destroy_cred,
	.crmatch	= nul_match,
	.crmarshal	= nul_marshal,
	.crrefresh	= nul_refresh,
	.crvalidate	= nul_validate,
};

static
struct rpc_cred null_cred = {
	.cr_ops		= &null_credops,
	.cr_count	= ATOMIC_INIT(1),
	.cr_flags	= RPCAUTH_CRED_UPTODATE,
#ifdef RPC_DEBUG
	.cr_magic	= RPCAUTH_CRED_MAGIC,
#endif
};


static int rpcproc_encode_null(void *rqstp, u32 *data, void *obj)
{
	return 0;
}

static int rpcproc_decode_null(void *rqstp, u32 *data, void *obj)
{
	return 0;
}

static struct rpc_procinfo rpcproc_null = {
	.p_encode = rpcproc_encode_null,
	.p_decode = rpcproc_decode_null,
};

static int rpc_ping(struct rpc_clnt *clnt, int flags)
{
	struct rpc_message msg = {
		.rpc_proc = &rpcproc_null,
	};
	int err;
	msg.rpc_cred = nul_lookup_cred(NULL, NULL, 0);
	err = rpc_call_sync(clnt, &msg, flags);
	put_rpccred(msg.rpc_cred);
	return err;
}

/**
 * rpc_bind_new_program - bind a new RPC program to an existing client
 * @old - old rpc_client
 * @program - rpc program to set
 * @vers - rpc program version
 *
 * Clones the rpc client and sets up a new RPC program. This is mainly
 * of use for enabling different RPC programs to share the same transport.
 * The Sun NFSv2/v3 ACL protocol can do this.
 */
#define RPC_TASK_NOINTR		0x0400		/* uninterruptible task */
static struct rpc_clnt *rpc_bind_new_program(struct rpc_clnt *old,
				      struct rpc_program *program,
				      int vers)
{
	struct rpc_clnt *clnt;
	struct rpc_version *version;
	int err;

	BUG_ON(vers >= program->nrvers || !program->version[vers]);
	version = program->version[vers];
	clnt = rpc_clone_client(old);
	if (IS_ERR(clnt))
		goto out;
	clnt->cl_procinfo = version->procs;
	clnt->cl_maxproc  = version->nrprocs;
	clnt->cl_protname = program->name;
	clnt->cl_prog     = program->number;
	clnt->cl_vers     = version->number;
	clnt->cl_stats    = program->stats;
	err = rpc_ping(clnt,RPC_TASK_SOFT|RPC_TASK_NOINTR);


	if (err != 0) {
		rpc_shutdown_client(clnt);
		clnt = ERR_PTR(err);
	}
out:	
	return clnt;
}

#endif

static void nfs_clear_inode(struct inode *);
static void nfs_umount_begin(struct super_block *);
static int  nfs_statfs(struct super_block *, struct kstatfs *);
static int  nfs_show_options(struct seq_file *, struct vfsmount *);
static void nfs_zap_acl_cache(struct inode *);

static struct super_operations nfs_sops = { 
	.alloc_inode	= nfs_alloc_inode,
	.destroy_inode	= nfs_destroy_inode,
	.write_inode	= nfs_write_inode,
	.delete_inode	= nfs_delete_inode,
#ifdef __SUSE9_SUP
	.put_super	= nfs_put_super,
#endif
	.statfs		= nfs_statfs,
	.clear_inode	= nfs_clear_inode,
	.umount_begin	= nfs_umount_begin,
	.show_options	= nfs_show_options,
};

/*
 * RPC cruft for NFS
 */
struct rpc_stat			nfs_rpcstat = {
	.program		= &nfs_program
};
static struct rpc_version *	nfs_version[] = {
	NULL,
	NULL,
	&nfs_version2,
#if defined(CONFIG_NFS_V3)
	&nfs_version3,
#elif defined(CONFIG_NFS_V4)
	NULL,
#endif
#if defined(CONFIG_NFS_V4)
	&nfs_version4,
#endif
};

struct rpc_program		nfs_program = {
	.name			= "nfs",
	.number			= NFS_PROGRAM,
	.nrvers			= sizeof(nfs_version) / sizeof(nfs_version[0]),
	.version		= nfs_version,
	.stats			= &nfs_rpcstat,
	.pipe_dir_name		= "/nfs",
};

#if defined(__SUSE9_SUP) && defined(CONFIG_NFS_ACL)
static struct rpc_stat		nfsacl_rpcstat = { &nfsacl_program };
static struct rpc_version *	nfsacl_version[] = {
	[3]			= &nfsacl_version3,
};

struct rpc_program		nfsacl_program = {
	.name =			"nfsacl",
	.number =		NFS_ACL_PROGRAM,
	.nrvers =		sizeof(nfsacl_version) / sizeof(nfsacl_version[0]),
	.version =		nfsacl_version,
	.stats =		&nfsacl_rpcstat,
};
#else
#ifdef CONFIG_NFS_V3_ACL
static struct rpc_stat		nfsacl_rpcstat = { &nfsacl_program };
static struct rpc_version *	nfsacl_version[] = {
	[3]			= &nfsacl_version3,
};

struct rpc_program		nfsacl_program = {
	.name =			"nfsacl",
	.number =		NFS_ACL_PROGRAM,
	.nrvers =		sizeof(nfsacl_version) / sizeof(nfsacl_version[0]),
	.version =		nfsacl_version,
	.stats =		&nfsacl_rpcstat,
};
#endif  /* CONFIG_NFS_ACL */
#endif  /*__SUSE9_SUP and CONFIG_NFS_ACL*/

/* qinping mod 20060331 begin */
/* zdz mod 20061204 begin*/
/* integrate with VSDS*/
#define ENFS_MOUNT_BLOCK_SIZE 4096
/* zdz mod 20061204 end*/

#ifdef ENFS_TEST

struct enfs_bcache_stat_t bcache_stat;
int t_debug_level = TDEBUG1;

void
enfs_invalidate_inode(struct inode *inode)
{
	nfs_zap_caches(inode);
}

#endif	/* ENFS_TEST */
/* qinping mod 20060331 end */

static inline unsigned long
nfs_fattr_to_ino_t(struct nfs_fattr *fattr)
{
	return nfs_fileid_to_ino_t(fattr->fileid);
}

static int
nfs_write_inode(struct inode *inode, int sync)
{
	int flags = sync ? FLUSH_WAIT : 0;
	int ret;

	/* qinping mod 20060331 begin */
	return 0;
	/* qinping mod 20060331 end */

	ret = nfs_commit_inode(inode, 0, 0, flags);
	if (ret < 0)
		return ret;
	return 0;
}

static void
nfs_delete_inode(struct inode * inode)
{
	dprintk("NFS: delete_inode(%s/%ld)\n", inode->i_sb->s_id, inode->i_ino);

	/* qinping mod 20060331 begin */
#if 0
	nfs_wb_all(inode);
	/*
	 * The following should never happen...
	 */
	if (nfs_have_writebacks(inode)) {
		printk(KERN_ERR "nfs_delete_inode: inode %ld has pending RPC requests\n", inode->i_ino);
	}
#endif
	/* qinping mod 20060331 end */
	clear_inode(inode);
}

/*
 * For the moment, the only task for the NFS clear_inode method is to
 * release the mmap credential
 */
static void
nfs_clear_inode(struct inode *inode)
{
	struct nfs_inode *nfsi = NFS_I(inode);
	struct rpc_cred *cred;

	/* qinping mod 20060331 begin */
	//nfs_wb_all(inode);
	if(NFS_FLAGS(inode) & NFS_INO_SYNC_SIZE)
		inode_attrset_exec(inode, 0);
	if (nfs_have_writebacks(inode)) {
		printk(KERN_ERR "nfs_delete_inode: inode %ld has pending RPC requests\n", inode->i_ino);
	}
	/* qinping mod 20060331 end */
	BUG_ON (!list_empty(&nfsi->open_files));
	nfs_zap_acl_cache(inode);
	cred = nfsi->cache_access.cred;
	if (cred)
		put_rpccred(cred);
	BUG_ON(atomic_read(&nfsi->data_updates) != 0);
}
#ifdef __SUSE9_SUP
void
nfs_put_super(struct super_block *sb)
{
	struct nfs_server *server = NFS_SB(sb);

	nfs4_renewd_prepare_shutdown(server);

	if (server->client != NULL)
		rpc_shutdown_client(server->client);

#ifdef CONFIG_NFS_ACL
	if (server->client_acl != NULL)
		rpc_shutdown_client(server->client_acl);
#endif  /* CONFIG_NFS_ACL */

	if (server->client_sys != NULL)
		rpc_shutdown_client(server->client_sys);

	if (!(server->flags & NFS_MOUNT_NONLM))
		lockd_down();	/* release rpc.lockd */
	rpciod_down();		/* release rpciod */

	destroy_nfsv4_state(server);

	kfree(server->hostname);
}
#endif


void
nfs_umount_begin(struct super_block *sb)
{
	struct nfs_server *server = NFS_SB(sb);
	struct rpc_clnt	*rpc;

	/* -EIO all pending I/O */
	if ((rpc = server->client) != NULL)
		rpc_killall_tasks(rpc);
	rpc = NFS_SB(sb)->client_acl;
	if (!IS_ERR(rpc))
		rpc_killall_tasks(rpc);
}


static inline unsigned long
nfs_block_bits(unsigned long bsize, unsigned char *nrbitsp)
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
 * Calculate the number of 512byte blocks used.
 */
static inline unsigned long
nfs_calc_block_size(u64 tsize)
{
	loff_t used = (tsize + 511) >> 9;
	return (used > ULONG_MAX) ? ULONG_MAX : used;
}

/*
 * Compute and set NFS server blocksize
 */
static inline unsigned long
nfs_block_size(unsigned long bsize, unsigned char *nrbitsp)
{
	if (bsize < 1024)
		bsize = NFS_DEF_FILE_IO_BUFFER_SIZE;
	else if (bsize >= NFS_MAX_FILE_IO_BUFFER_SIZE)
		bsize = NFS_MAX_FILE_IO_BUFFER_SIZE;

	return nfs_block_bits(bsize, nrbitsp);
}

/*
 * Obtain the root inode of the file system.
 */
static struct inode *
nfs_get_root(struct super_block *sb, struct nfs_fh *rootfh, struct nfs_fsinfo *fsinfo)
{
	struct nfs_server	*server = NFS_SB(sb);
	struct inode *rooti;
	int			error;

	error = server->rpc_ops->getroot(server, rootfh, fsinfo);
	if (error < 0) {
		dprintk("nfs_get_root: getattr error = %d\n", -error);
		return ERR_PTR(error);
	}

	rooti = nfs_fhget(sb, rootfh, fsinfo->fattr);
	if (!rooti)
		return ERR_PTR(-ENOMEM);
	return rooti;
}

/*
 * Do NFS version-independent mount processing, and sanity checking
 */
static int
nfs_sb_init(struct super_block *sb, rpc_authflavor_t authflavor)
{
	struct nfs_server	*server;
	struct inode		*root_inode;
	struct nfs_fattr	fattr;
	struct nfs_fsinfo	fsinfo = {
					.fattr = &fattr,
				};
	struct nfs_pathconf pathinfo = {
			.fattr = &fattr,
	};
	int no_root_error = 0;

	/* We probably want something more informative here */
	snprintf(sb->s_id, sizeof(sb->s_id), "%x:%x", MAJOR(sb->s_dev), MINOR(sb->s_dev));

	server = NFS_SB(sb);
/*zdz add 20061205 begin*/
	pserver = server;
/*zdz add 20061205 end*/
	sb->s_magic      = NFS_SUPER_MAGIC;

	root_inode = nfs_get_root(sb, &server->fh, &fsinfo);
	/* Did getting the root inode fail? */
	if (IS_ERR(root_inode)) {
		no_root_error = PTR_ERR(root_inode);
		goto out_no_root;
	}
	sb->s_root = d_alloc_root(root_inode);
	if (!sb->s_root) {
		no_root_error = -ENOMEM;
		goto out_no_root;
	}
	sb->s_root->d_op = server->rpc_ops->dentry_ops;

	/* Get some general file system info */
	if (server->namelen == 0 &&
	    server->rpc_ops->pathconf(server, &server->fh, &pathinfo) >= 0)
		server->namelen = pathinfo.max_namelen;
	/* Work out a lot of parameters */
	if (server->rsize == 0)
		server->rsize = nfs_block_size(fsinfo.rtpref, NULL);
	if (server->wsize == 0)
		server->wsize = nfs_block_size(fsinfo.wtpref, NULL);

	if (fsinfo.rtmax >= 512 && server->rsize > fsinfo.rtmax)
		server->rsize = nfs_block_size(fsinfo.rtmax, NULL);
	if (fsinfo.wtmax >= 512 && server->wsize > fsinfo.wtmax)
		server->wsize = nfs_block_size(fsinfo.wtmax, NULL);

	server->rpages = (server->rsize + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;
	if (server->rpages > NFS_READ_MAXIOV) {
		server->rpages = NFS_READ_MAXIOV;
		server->rsize = server->rpages << PAGE_CACHE_SHIFT;
	}

	server->wpages = (server->wsize + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;
        if (server->wpages > NFS_WRITE_MAXIOV) {
		server->wpages = NFS_WRITE_MAXIOV;
                server->wsize = server->wpages << PAGE_CACHE_SHIFT;
	}

	if (sb->s_blocksize == 0)
		sb->s_blocksize = nfs_block_bits(server->wsize,
							 &sb->s_blocksize_bits);
	server->wtmult = nfs_block_bits(fsinfo.wtmult, NULL);

	server->dtsize = nfs_block_size(fsinfo.dtpref, NULL);
	if (server->dtsize > PAGE_CACHE_SIZE)
		server->dtsize = PAGE_CACHE_SIZE;
	if (server->dtsize > server->rsize)
		server->dtsize = server->rsize;

	if (server->flags & NFS_MOUNT_NOAC) {
		server->acregmin = server->acregmax = 0;
		server->acdirmin = server->acdirmax = 0;
		sb->s_flags |= MS_SYNCHRONOUS;
	}
/*zdz mod 20061205 begin*/
if(enfs_ra_flag)
	server->backing_dev_info.ra_pages = enfs_ra_pages;
else   
	server->backing_dev_info.ra_pages = server->rpages * NFS_MAX_READAHEAD;
/*zdz mod 20061205 end*/

	sb->s_maxbytes = fsinfo.maxfilesize;
	if (sb->s_maxbytes > MAX_LFS_FILESIZE) 
		sb->s_maxbytes = MAX_LFS_FILESIZE; 

	/* We're airborne Set socket buffersize */
	rpc_setbufsize(server->client, server->wsize + 100, server->rsize + 100);
	return 0;
	/* Yargs. It didn't work out. */
out_no_root:
	dprintk("nfs_sb_init: get root inode failed: errno %d\n", -no_root_error);
	if (!IS_ERR(root_inode))
		iput(root_inode);
	return no_root_error;
}

/*
 * Create an RPC client handle.
 */
static struct rpc_clnt *
nfs_create_client(struct nfs_server *server, const struct nfs_mount_data *data)
{
	struct rpc_timeout	timeparms;
	struct rpc_xprt		*xprt = NULL;
	struct rpc_clnt		*clnt = NULL;
	int			tcp   = (data->flags & NFS_MOUNT_TCP);

	/* Initialize timeout values */
	timeparms.to_initval = data->timeo * HZ / 10;
	timeparms.to_retries = data->retrans;
	timeparms.to_maxval  = tcp ? RPC_MAX_TCP_TIMEOUT : RPC_MAX_UDP_TIMEOUT;
	timeparms.to_exponential = 1;

	if (!timeparms.to_initval)
		timeparms.to_initval = (tcp ? 600 : 11) * HZ / 10;
	if (!timeparms.to_retries)
		timeparms.to_retries = 5;

	/* create transport and client */
	xprt = xprt_create_proto(tcp ? IPPROTO_TCP : IPPROTO_UDP,
				 &server->addr, &timeparms);
	if (IS_ERR(xprt)) {
		printk(KERN_WARNING "NFS: cannot create RPC transport.\n");
		return (struct rpc_clnt *)xprt;
	}
	clnt = rpc_create_client(xprt, server->hostname, &nfs_program,
				 server->rpc_ops->version, data->pseudoflavor);
	if (IS_ERR(clnt)) {
		printk(KERN_WARNING "NFS: cannot create RPC client.\n");
		goto out_fail;
	}

	clnt->cl_intr     = (server->flags & NFS_MOUNT_INTR) ? 1 : 0;
	clnt->cl_softrtry = (server->flags & NFS_MOUNT_SOFT) ? 1 : 0;
	clnt->cl_droppriv = (server->flags & NFS_MOUNT_BROKEN_SUID) ? 1 : 0;
	clnt->cl_chatty   = 1;

	return clnt;

out_fail:
	xprt_destroy(xprt);
	return clnt;
}

/*
static struct block_device *enfs_open_bdev(dev_t dev, void *holder)
{
	struct block_device *bdev;
	int rc;

	bdev = open_by_devnum(dev, FMODE_READ|FMODE_WRITE);
	if (IS_ERR(bdev))
		return bdev;
	rc = bd_claim(bdev, holder);
	if (rc) {
		blkdev_put(bdev);
		return ERR_PTR(rc);
	}
	rc = set_blocksize(bdev, ENFS_MOUNT_BLOCK_SIZE);
	if (rc) {
		bd_release(bdev);
		blkdev_put(bdev);
		return ERR_PTR(rc);
	}
	return bdev;
}

static void enfs_close_bdev(struct block_device *bdev)
{
	if (bdev)
		close_bdev_excl(bdev);
}
*/

/*
 * The way this works is that the mount process passes a structure
 * in the data argument which contains the server's IP address
 * and the root file handle obtained from the server's mount
 * daemon. We stash these away in the private superblock fields.
 */
static int
nfs_fill_super(struct super_block *sb, struct nfs_mount_data *data, int silent)
{
	struct nfs_server	*server;
	rpc_authflavor_t	authflavor;

	server           = NFS_SB(sb);
	sb->s_blocksize_bits = 0;
	sb->s_blocksize = 0;
	if (data->bsize)
		sb->s_blocksize = nfs_block_size(data->bsize, &sb->s_blocksize_bits);
	if (data->rsize)
		server->rsize = nfs_block_size(data->rsize, NULL);
	if (data->wsize)
		server->wsize = nfs_block_size(data->wsize, NULL);
	server->flags    = data->flags & NFS_MOUNT_FLAGMASK;

	server->acregmin = data->acregmin*HZ;
	server->acregmax = data->acregmax*HZ;
	server->acdirmin = data->acdirmin*HZ;
	server->acdirmax = data->acdirmax*HZ;

	/* Start lockd here, before we might error out */
	if (!(server->flags & NFS_MOUNT_NONLM))
	/* qinping mod 20060331 begin */
	{
		//lockd_up();
	}
	/* qinping mod 20060331 end */

	server->namelen  = data->namlen;
	server->hostname = kmalloc(strlen(data->hostname) + 1, GFP_KERNEL);
	if (!server->hostname)
		return -ENOMEM;
	strcpy(server->hostname, data->hostname);

	/* Check NFS protocol revision and initialize RPC op vector
	 * and file handle pool. */
	if (server->flags & NFS_MOUNT_VER3) {
#ifdef CONFIG_NFS_V3
		server->rpc_ops = &nfs_v3_clientops;
		server->caps |= NFS_CAP_READDIRPLUS;
		if (data->version < 4) {
			printk(KERN_NOTICE "NFS: NFSv3 not supported by mount program.\n");
			return -EIO;
		}
#else
		printk(KERN_NOTICE "NFS: NFSv3 not supported.\n");
		return -EIO;
#endif
	} else {
		server->rpc_ops = &nfs_v2_clientops;
	}

	/* Fill in pseudoflavor for mount version < 5 */
	if (!(data->flags & NFS_MOUNT_SECFLAVOUR))
		data->pseudoflavor = RPC_AUTH_UNIX;
	authflavor = data->pseudoflavor;	/* save for sb_init() */
	/* XXX maybe we want to add a server->pseudoflavor field */

	/* Create RPC client handles */
	server->client = nfs_create_client(server, data);
	if (IS_ERR(server->client))
		return PTR_ERR(server->client);
	/* RFC 2623, sec 2.3.2 */
	if (authflavor != RPC_AUTH_UNIX) {
		server->client_sys = rpc_clone_client(server->client);
		if (IS_ERR(server->client_sys))
			return PTR_ERR(server->client_sys);
		if (!rpcauth_create(RPC_AUTH_UNIX, server->client_sys))
			return -ENOMEM;
	} else {
		atomic_inc(&server->client->cl_count);
		server->client_sys = server->client;
	}
	if (server->flags & NFS_MOUNT_VER3) {
#ifdef __SUSE9_SUP 
#ifdef CONFIG_NFS_ACL
		if (!(server->flags & NFS_MOUNT_NOACL)) {
			server->client_acl = rpc_bind_new_program(server->client, &nfsacl_program, 3);
			/* No errors! Assume that Sun nfsacls are supported */
			if (!IS_ERR(server->client_acl))
				server->caps |= NFS_CAP_ACLS;
		}
#else
		server->flags &= ~NFS_MOUNT_NOACL;




#endif /* CONFIG_NFS_ACL */

#endif /*__SUSE9_SUP*/
#ifndef __SUSE9_SUP
#ifdef CONFIG_NFS_V3_ACL
		if (!(server->flags & NFS_MOUNT_NOACL)) {
			server->client_acl = rpc_bind_new_program(server->client, &nfsacl_program, 3);
			/* No errors! Assume that Sun nfsacls are supported */
			if (!IS_ERR(server->client_acl))
				server->caps |= NFS_CAP_ACLS;
		}
#else
		server->flags &= ~NFS_MOUNT_NOACL;
#endif /* CONFIG_NFS_V3_ACL */
#endif  /*__SUSE9_SUP*/
		/*
		 * The VFS shouldn't apply the umask to mode bits. We will
		 * do so ourselves when necessary.
		 */
		sb->s_flags |= MS_POSIXACL;
		if (server->namelen == 0 || server->namelen > NFS3_MAXNAMLEN)
			server->namelen = NFS3_MAXNAMLEN;
	} else {
		if (server->namelen == 0 || server->namelen > NFS2_MAXNAMLEN)
			server->namelen = NFS2_MAXNAMLEN;
	}

	sb->s_blocksize = nfs_block_bits(ENFS_MOUNT_BLOCK_SIZE, &sb->s_blocksize_bits);

	if(sb->s_blocksize != ENFS_MOUNT_BLOCK_SIZE)
		printk(KERN_ERR "blocksize is ERROR, should be %d, but now = %d\n", ENFS_MOUNT_BLOCK_SIZE, (int)sb->s_blocksize);

	sb->s_op = &nfs_sops;
	return nfs_sb_init(sb, authflavor);
}

static int
nfs_statfs(struct super_block *sb, struct kstatfs *buf)
{
	struct nfs_server *server = NFS_SB(sb);
	unsigned char blockbits;
	unsigned long blockres;
	struct nfs_fh *rootfh = NFS_FH(sb->s_root->d_inode);
	struct nfs_fattr fattr;
	struct nfs_fsstat res = {
			.fattr = &fattr,
	};
	int error;

	lock_kernel();

	error = server->rpc_ops->statfs(server, rootfh, &res);
	buf->f_type = NFS_SUPER_MAGIC;
	if (error < 0)
		goto out_err;

	buf->f_frsize = server->wtmult;
	buf->f_bsize = sb->s_blocksize;
	blockbits = sb->s_blocksize_bits;
	blockres = (1 << blockbits) - 1;
	buf->f_blocks = (res.tbytes + blockres) >> blockbits;
	buf->f_bfree = (res.fbytes + blockres) >> blockbits;
	buf->f_bavail = (res.abytes + blockres) >> blockbits;
	buf->f_files = res.tfiles;
	buf->f_ffree = res.afiles;

	buf->f_namelen = server->namelen;
 out:
	unlock_kernel();

	return 0;

 out_err:
	printk(KERN_WARNING "nfs_statfs: statfs error = %d\n", -error);
	buf->f_bsize = buf->f_blocks = buf->f_bfree = buf->f_bavail = -1;
	goto out;

}

static int nfs_show_options(struct seq_file *m, struct vfsmount *mnt)
{
	static struct proc_nfs_info {
		int flag;
		char *str;
		char *nostr;
	} nfs_info[] = {
		{ NFS_MOUNT_SOFT, ",soft", ",hard" },
		{ NFS_MOUNT_INTR, ",intr", "" },
		{ NFS_MOUNT_POSIX, ",posix", "" },
		{ NFS_MOUNT_TCP, ",tcp", ",udp" },
		{ NFS_MOUNT_NOCTO, ",nocto", "" },
		{ NFS_MOUNT_NOAC, ",noac", "" },
		{ NFS_MOUNT_NONLM, ",nolock", ",lock" },
		{ NFS_MOUNT_NOACL, ",noacl", "" },
		{ NFS_MOUNT_BROKEN_SUID, ",broken_suid", "" },
		{ 0, NULL, NULL }
	};
	struct proc_nfs_info *nfs_infop;
	struct nfs_server *nfss = NFS_SB(mnt->mnt_sb);

	seq_printf(m, ",v%d", nfss->rpc_ops->version);
	seq_printf(m, ",rsize=%d", nfss->rsize);
	seq_printf(m, ",wsize=%d", nfss->wsize);
	if (nfss->acregmin != 3*HZ)
		seq_printf(m, ",acregmin=%d", nfss->acregmin/HZ);
	if (nfss->acregmax != 60*HZ)
		seq_printf(m, ",acregmax=%d", nfss->acregmax/HZ);
	if (nfss->acdirmin != 30*HZ)
		seq_printf(m, ",acdirmin=%d", nfss->acdirmin/HZ);
	if (nfss->acdirmax != 60*HZ)
		seq_printf(m, ",acdirmax=%d", nfss->acdirmax/HZ);
	for (nfs_infop = nfs_info; nfs_infop->flag; nfs_infop++) {
		if (nfss->flags & nfs_infop->flag)
			seq_puts(m, nfs_infop->str);
		else
			seq_puts(m, nfs_infop->nostr);
	}
	seq_puts(m, ",addr=");
	seq_escape(m, nfss->hostname, " \t\n\\");
	return 0;
}

/*
 * Invalidate the local caches
 */
void nfs_zap_caches_locked(struct inode *inode)
{
	struct nfs_inode *nfsi = NFS_I(inode);
	int mode = inode->i_mode;

	NFS_ATTRTIMEO(inode) = NFS_MINATTRTIMEO(inode);
	NFS_ATTRTIMEO_UPDATE(inode) = jiffies;

	/* qinping mod 20060331 begin */
	if(S_ISREG(mode))
		enfs_init_bcache(inode);
	/* qinping mod 20060331 end */

	memset(NFS_COOKIEVERF(inode), 0, sizeof(NFS_COOKIEVERF(inode)));
	if (S_ISREG(mode) || S_ISDIR(mode) || S_ISLNK(mode))
		nfsi->cache_validity |= NFS_INO_INVALID_ATTR|NFS_INO_INVALID_DATA|NFS_INO_INVALID_ACCESS|NFS_INO_INVALID_ACL|NFS_INO_REVAL_PAGECACHE;
	else
		nfsi->cache_validity |= NFS_INO_INVALID_ATTR|NFS_INO_INVALID_ACCESS|NFS_INO_INVALID_ACL|NFS_INO_REVAL_PAGECACHE;
}

void nfs_zap_caches(struct inode *inode)
{
	down(&NFS_MAPSEM(inode));
	spin_lock(&inode->i_lock);
	nfs_zap_caches_locked(inode);
	spin_unlock(&inode->i_lock);
	up(&NFS_MAPSEM(inode));
}

static void nfs_zap_acl_cache(struct inode *inode)
{
	void (*clear_acl_cache)(struct inode *);

	clear_acl_cache = NFS_PROTO(inode)->clear_acl_cache;
	if (clear_acl_cache != NULL)
		clear_acl_cache(inode);
	spin_lock(&inode->i_lock);
	NFS_I(inode)->cache_validity &= ~NFS_INO_INVALID_ACL;
	spin_unlock(&inode->i_lock);
}

/*
 * Invalidate, but do not unhash, the inode
 */
static void
nfs_invalidate_inode(struct inode *inode)
{
	umode_t save_mode = inode->i_mode;

	make_bad_inode(inode);
	inode->i_mode = save_mode;
	nfs_zap_caches_locked(inode);
}

struct nfs_find_desc {
	struct nfs_fh		*fh;
	struct nfs_fattr	*fattr;
};

/*
 * In NFSv3 we can have 64bit inode numbers. In order to support
 * this, and re-exported directories (also seen in NFSv2)
 * we are forced to allow 2 different inodes to have the same
 * i_ino.
 */
static int
nfs_find_actor(struct inode *inode, void *opaque)
{
	struct nfs_find_desc	*desc = (struct nfs_find_desc *)opaque;
	struct nfs_fh		*fh = desc->fh;
	struct nfs_fattr	*fattr = desc->fattr;

	if (NFS_FILEID(inode) != fattr->fileid)
		return 0;
	if (nfs_compare_fh(NFS_FH(inode), fh))
		return 0;
	if (is_bad_inode(inode))
		return 0;
	return 1;
}

static int
nfs_init_locked(struct inode *inode, void *opaque)
{
	struct nfs_find_desc	*desc = (struct nfs_find_desc *)opaque;
	struct nfs_fattr	*fattr = desc->fattr;

	NFS_FILEID(inode) = fattr->fileid;
	nfs_copy_fh(NFS_FH(inode), desc->fh);
	return 0;
}

/* Don't use READDIRPLUS on directories that we believe are too large */
#define NFS_LIMIT_READDIRPLUS (8*PAGE_SIZE)

/*
 * This is our front-end to iget that looks up inodes by file handle
 * instead of inode number.
 */
struct inode *
nfs_fhget(struct super_block *sb, struct nfs_fh *fh, struct nfs_fattr *fattr)
{
	struct nfs_find_desc desc = {
		.fh	= fh,
		.fattr	= fattr
	};
	struct inode *inode = NULL;
	unsigned long hash;

	if ((fattr->valid & NFS_ATTR_FATTR) == 0)
		goto out_no_inode;

	if (!fattr->nlink) {
		printk("NFS: Buggy server - nlink == 0!\n");
		goto out_no_inode;
	}

	hash = nfs_fattr_to_ino_t(fattr);

	if (!(inode = iget5_locked(sb, hash, nfs_find_actor, nfs_init_locked, &desc)))
		goto out_no_inode;

	if (inode->i_state & I_NEW) {
		struct nfs_inode *nfsi = NFS_I(inode);

		/* We set i_ino for the few things that still rely on it,
		 * such as stat(2) */
		inode->i_ino = hash;

		/* We can't support update_atime(), since the server will reset it */
		inode->i_flags |= S_NOATIME|S_NOCMTIME;
		inode->i_mode = fattr->mode;
		/* Why so? Because we want revalidate for devices/FIFOs, and
		 * that's precisely what we have in nfs_file_inode_operations.
		 */
		inode->i_op = NFS_SB(sb)->rpc_ops->file_inode_ops;
		if (S_ISREG(inode->i_mode)) {
			inode->i_fop = &nfs_file_operations;
			inode->i_data.a_ops = &nfs_file_aops;
			inode->i_data.backing_dev_info = &NFS_SB(sb)->backing_dev_info;
#ifdef CONFIG_HIGHMEM
			/*
			 * Without a sound congestion-controlled writeback
			 * mechanism, using HIGHMEM would cause OOM on
			 * machines with lots of HIGHMEM.
			 */
			mapping_set_gfp_mask(&inode->i_data, GFP_KERNEL);
#endif
		} else if (S_ISDIR(inode->i_mode)) {
			inode->i_op = NFS_SB(sb)->rpc_ops->dir_inode_ops;
			inode->i_fop = &nfs_dir_operations;
			if (nfs_server_capable(inode, NFS_CAP_READDIRPLUS)
			    && fattr->size <= NFS_LIMIT_READDIRPLUS)
				NFS_FLAGS(inode) |= NFS_INO_ADVISE_RDPLUS;
		} else if (S_ISLNK(inode->i_mode))
			inode->i_op = &nfs_symlink_inode_operations;
		else
			init_special_inode(inode, inode->i_mode, fattr->rdev);

		nfsi->read_cache_jiffies = fattr->time_start;
		nfsi->last_updated = jiffies;
		inode->i_atime = fattr->atime;
		inode->i_mtime = fattr->mtime;
		inode->i_ctime = fattr->ctime;
		if (fattr->valid & NFS_ATTR_FATTR_V4)
			nfsi->change_attr = fattr->change_attr;
		inode->i_size = nfs_size_to_loff_t(fattr->size);
		inode->i_nlink = fattr->nlink;
		inode->i_uid = fattr->uid;
		inode->i_gid = fattr->gid;
		/* qinping mod 20060331 begin */
		//inode->i_bdev = inode->i_sb->s_bdev;
		//inode->i_rdev = inode->i_sb->s_dev;
		/* qinping mod 20060331 end */
		if (fattr->valid & (NFS_ATTR_FATTR_V3 | NFS_ATTR_FATTR_V4)) {
			/*
			 * report the blocks in 512byte units
			 */
			inode->i_blocks = nfs_calc_block_size(fattr->du.nfs3.used);
			inode->i_blksize = inode->i_sb->s_blocksize;
			/* qinping mod 20060331 begin */
			inode->i_blkbits = inode->i_sb->s_blocksize_bits;
			/* qinping mod 20060331 end */
		} else {
			inode->i_blocks = fattr->du.nfs2.blocks;
			inode->i_blksize = fattr->du.nfs2.blocksize;
		}
		nfsi->attrtimeo = NFS_MINATTRTIMEO(inode);
		nfsi->attrtimeo_timestamp = jiffies;
		memset(nfsi->cookieverf, 0, sizeof(nfsi->cookieverf));
		nfsi->cache_access.cred = NULL;

		/* qinping mod 20060331 begin */
		init_MUTEX(&nfsi->m_sem);
		nfsi->victim = 0;
		nfsi->mru = 0;
		memset(&nfsi->pbcache, 0, sizeof(struct bcache));
		INIT_LIST_HEAD(&nfsi->attrset);
		nfsi->attrset_timeout = 0;
		/* qinping mod 20060331 end */

		unlock_new_inode(inode);
	} else
		nfs_refresh_inode(inode, fattr);
	dprintk("NFS: nfs_fhget(%s/%Ld ct=%d)\n",
		inode->i_sb->s_id,
		(long long)NFS_FILEID(inode),
		atomic_read(&inode->i_count));

out:
	return inode;

out_no_inode:
	printk("nfs_fhget: iget failed\n");
	goto out;
}

/* qinping mod 20060331 begin */
//#define NFS_VALID_ATTRS (ATTR_MODE|ATTR_UID|ATTR_GID|ATTR_SIZE|ATTR_ATIME|ATTR_ATIME_SET|ATTR_MTIME|ATTR_MTIME_SET)
#define NFS_VALID_ATTRS (ATTR_MODE|ATTR_UID|ATTR_GID|ATTR_SIZE|ATTR_ATIME|ATTR_ATIME_SET|ATTR_MTIME|ATTR_MTIME_SET|ATTR_WRITE)
/* qinping mod 20060331 end */

int
nfs_setattr(struct dentry *dentry, struct iattr *attr)
{
	struct inode *inode = dentry->d_inode;
	struct nfs_fattr fattr;
	int error;
	/* qinping mod 20060331 begin */
	int has_attr_write = 0;
	/* qinping mod 20060331 end */

	/*
	 * If the size of the inode has yet to be synchronized, we
	 * merge that request with "attr".
	 */
	if (NFS_FLAGS(inode) & NFS_INO_SYNC_SIZE) {
		has_attr_write = 1;
		/*
		 * ATTR_SIZE and ATTR_WRITE are mutually exclusive.
		 */
		if (!(attr->ia_valid & ATTR_SIZE)) {
			attr->ia_size = i_size_read(inode);
			attr->ia_valid |= ATTR_WRITE;
		}
		attr->ia_valid |= ATTR_MTIME;
	}

	if (attr->ia_valid & ATTR_SIZE) {
		/*
		 * If an ATTR_WRITE has been merged in, ATTR_SIZE
		 * should never be optimized away.
		 */
		if (!S_ISREG(inode->i_mode) ||
		    (!has_attr_write && attr->ia_size == i_size_read(inode)))
			attr->ia_valid &= ~ATTR_SIZE;
	}

	/* Optimization: if the end result is no change, don't RPC */
	attr->ia_valid &= NFS_VALID_ATTRS;
	if (attr->ia_valid == 0)
		return 0;

	lock_kernel();
	/* qinping mod 20060331 begin */
	if (NFS_FLAGS(inode) & NFS_INO_SYNC_SIZE)
		inode_attrset_del(inode);
	/* qinping mod 20060331 end */
	nfs_begin_data_update(inode);
	/* Write all dirty data if we're changing file permissions or size */
	/* qinping mod 20060331 begin */
	NFS_FLAGS(inode) |= NFS_INO_SETTING;

	//if ((attr->ia_valid & (ATTR_MODE|ATTR_UID|ATTR_GID|ATTR_SIZE)) != 0) { 
	if ((attr->ia_valid & (ATTR_MODE|ATTR_UID|ATTR_GID|ATTR_SIZE|ATTR_WRITE)) != 0) { 
	/* qinping mod 20060331 end */
		if (filemap_fdatawrite(inode->i_mapping) == 0)
			filemap_fdatawait(inode->i_mapping);
		nfs_wb_all(inode);
	}
	error = NFS_PROTO(inode)->setattr(dentry, &fattr, attr);
	/* qinping mod 20060331 begin */
#ifdef ENFS_TEST
	if (atomic_read(&enfs_saerror))
		error = -EIO;
#endif
	/* qinping mod 20060331 end */
	if (error == 0)
		nfs_refresh_inode(inode, &fattr);
	/* qinping mod 20060331 begin */
	else if (has_attr_write)
		/*
		 * We can't give up ATTR_WRITEs.
		 */
		inode_attrset_insert(inode);
	NFS_FLAGS(inode) &= ~NFS_INO_SETTING;
	/* qinping mod 20060331 end */
	nfs_end_data_update(inode);
	unlock_kernel();
	return error;
}

/**
 * nfs_setattr_update_inode - Update inode metadata after a setattr call.
 * @inode: pointer to struct inode
 * @attr: pointer to struct iattr
 *
 * Note: we do this in the *proc.c in order to ensure that
 *       it works for things like exclusive creates too.
 */
void nfs_setattr_update_inode(struct inode *inode, struct iattr *attr)
{
	if ((attr->ia_valid & (ATTR_MODE|ATTR_UID|ATTR_GID)) != 0) {
		if ((attr->ia_valid & ATTR_MODE) != 0) {
			int mode = attr->ia_mode & S_IALLUGO;
			mode |= inode->i_mode & ~S_IALLUGO;
			inode->i_mode = mode;
		}
		if ((attr->ia_valid & ATTR_UID) != 0)
			inode->i_uid = attr->ia_uid;
		if ((attr->ia_valid & ATTR_GID) != 0)
			inode->i_gid = attr->ia_gid;
		spin_lock(&inode->i_lock);
		NFS_I(inode)->cache_validity |= NFS_INO_INVALID_ACCESS|NFS_INO_INVALID_ACL;
		spin_unlock(&inode->i_lock);
	}
	if ((attr->ia_valid & ATTR_SIZE) != 0) {
		inode->i_size = attr->ia_size;
		/* qinping mod 20060331 begin */
		klog(DEBUG, "need_resched = %d\n", need_resched());
		/* qinping mod 20060331 end */
		vmtruncate(inode, attr->ia_size);
		/* qinping mod 20060331 begin */
		enfs_check_bcache(inode, attr->ia_size);
		/* qinping mod 20060331 end */
	}
}

/*
 * Wait for the inode to get unlocked.
 * (Used for NFS_INO_LOCKED and NFS_INO_REVALIDATING).
 */
int
nfs_wait_on_inode(struct inode *inode, int flag)
{
	struct rpc_clnt	*clnt = NFS_CLIENT(inode);
	struct nfs_inode *nfsi = NFS_I(inode);

	int error;
	if (!(NFS_FLAGS(inode) & flag))
		return 0;
	atomic_inc(&inode->i_count);
	error = nfs_wait_event(clnt, nfsi->nfs_i_wait,
				!(NFS_FLAGS(inode) & flag));
	iput(inode);
	return error;
}

int nfs_getattr(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat)
{
	struct inode *inode = dentry->d_inode;
	int need_atime = NFS_I(inode)->cache_validity & NFS_INO_INVALID_ATIME;
	int err;

	if (__IS_FLG(inode, MS_NOATIME))
		need_atime = 0;
	else if (__IS_FLG(inode, MS_NODIRATIME) && S_ISDIR(inode->i_mode))
		need_atime = 0;
	/* We may force a getattr if the user cares about atime */
	if (need_atime)
		err = __nfs_revalidate_inode(NFS_SERVER(inode), inode);
	else
		err = nfs_revalidate_inode(NFS_SERVER(inode), inode);
	if (!err)
		generic_fillattr(inode, stat);
	return err;
}

struct nfs_open_context *alloc_nfs_open_context(struct dentry *dentry, struct rpc_cred *cred)
{
	struct nfs_open_context *ctx;

	ctx = (struct nfs_open_context *)kmalloc(sizeof(*ctx), GFP_KERNEL);
	if (ctx != NULL) {
		atomic_set(&ctx->count, 1);
		ctx->dentry = dget(dentry);
		ctx->cred = get_rpccred(cred);
		ctx->state = NULL;
		ctx->lockowner = current->files;
		ctx->error = 0;
/* liw add 20061120 begin */
#ifdef __DIR_F_POS_INDEX
		ctx->dir_cookie = 0;
#endif
/* liw add 20061120 end */
		init_waitqueue_head(&ctx->waitq);
	}
	return ctx;
}

struct nfs_open_context *get_nfs_open_context(struct nfs_open_context *ctx)
{
	if (ctx != NULL)
		atomic_inc(&ctx->count);
	return ctx;
}

void put_nfs_open_context(struct nfs_open_context *ctx)
{
	if (atomic_dec_and_test(&ctx->count)) {
		if (ctx->state != NULL)
			nfs4_close_state(ctx->state, ctx->mode);
		if (ctx->cred != NULL)
			put_rpccred(ctx->cred);
		dput(ctx->dentry);
		kfree(ctx);
	}
}

/*
 * Ensure that mmap has a recent RPC credential for use when writing out
 * shared pages
 */
void nfs_file_set_open_context(struct file *filp, struct nfs_open_context *ctx)
{
	struct inode *inode = filp->f_dentry->d_inode;
	struct nfs_inode *nfsi = NFS_I(inode);

	filp->private_data = get_nfs_open_context(ctx);
	spin_lock(&inode->i_lock);
	list_add(&ctx->list, &nfsi->open_files);
	spin_unlock(&inode->i_lock);
}

struct nfs_open_context *nfs_find_open_context(struct inode *inode, int mode)
{
	struct nfs_inode *nfsi = NFS_I(inode);
	struct nfs_open_context *pos, *ctx = NULL;

	spin_lock(&inode->i_lock);
	list_for_each_entry(pos, &nfsi->open_files, list) {
		if ((pos->mode & mode) == mode) {
			ctx = get_nfs_open_context(pos);
			break;
		}
	}
	spin_unlock(&inode->i_lock);
	return ctx;
}

void nfs_file_clear_open_context(struct file *filp)
{
	struct inode *inode = filp->f_dentry->d_inode;
	struct nfs_open_context *ctx = (struct nfs_open_context *)filp->private_data;

	if (ctx) {
		filp->private_data = NULL;
		spin_lock(&inode->i_lock);
		list_del(&ctx->list);
		spin_unlock(&inode->i_lock);
		put_nfs_open_context(ctx);
	}
}

/*
 * These allocate and release file read/write context information.
 */
int nfs_open(struct inode *inode, struct file *filp)
{
	struct nfs_open_context *ctx;
	struct rpc_cred *cred;

	if ((cred = rpcauth_lookupcred(NFS_CLIENT(inode)->cl_auth, 0)) == NULL)
		return -ENOMEM;
	ctx = alloc_nfs_open_context(filp->f_dentry, cred);
	put_rpccred(cred);
	if (ctx == NULL)
		return -ENOMEM;
	ctx->mode = filp->f_mode;
	nfs_file_set_open_context(filp, ctx);
	put_nfs_open_context(ctx);
	if ((filp->f_mode & FMODE_WRITE) != 0)
		nfs_begin_data_update(inode);
	return 0;
}

int nfs_release(struct inode *inode, struct file *filp)
{
	if ((filp->f_mode & FMODE_WRITE) != 0)
		nfs_end_data_update(inode);
	nfs_file_clear_open_context(filp);
	return 0;
}

/*
 * This function is called whenever some part of NFS notices that
 * the cached attributes have to be refreshed.
 */
int
__nfs_revalidate_inode(struct nfs_server *server, struct inode *inode)
{
	int		 status = -ESTALE;
	struct nfs_fattr fattr;
	struct nfs_inode *nfsi = NFS_I(inode);
	unsigned long verifier;
	unsigned long cache_validity;

	dfprintk(PAGECACHE, "NFS: revalidating (%s/%Ld)\n",
		inode->i_sb->s_id, (long long)NFS_FILEID(inode));

	lock_kernel();
	if (!inode || is_bad_inode(inode))
 		goto out_nowait;
	if (NFS_STALE(inode) && inode != inode->i_sb->s_root->d_inode)
 		goto out_nowait;

	while (NFS_REVALIDATING(inode)) {
		status = nfs_wait_on_inode(inode, NFS_INO_REVALIDATING);
		if (status < 0)
			goto out_nowait;
		if (NFS_SERVER(inode)->flags & NFS_MOUNT_NOAC)
			continue;
		if (nfsi->cache_validity & (NFS_INO_INVALID_ATTR|NFS_INO_INVALID_DATA|NFS_INO_INVALID_ATIME))
			continue;
		status = NFS_STALE(inode) ? -ESTALE : 0;
		goto out_nowait;
	}
	NFS_FLAGS(inode) |= NFS_INO_REVALIDATING;

	/* Protect against RPC races by saving the change attribute */
	verifier = nfs_save_change_attribute(inode);
	status = NFS_PROTO(inode)->getattr(server, NFS_FH(inode), &fattr);
	if (status) {
		dfprintk(PAGECACHE, "nfs_revalidate_inode: (%s/%Ld) getattr failed, error=%d\n",
			 inode->i_sb->s_id,
			 (long long)NFS_FILEID(inode), status);
		if (status == -ESTALE) {
			NFS_FLAGS(inode) |= NFS_INO_STALE;
			if (inode != inode->i_sb->s_root->d_inode)
				remove_inode_hash(inode);
		}
		goto out;
	}

	down(&NFS_MAPSEM(inode));
	spin_lock(&inode->i_lock);
	status = nfs_update_inode(inode, &fattr, verifier);
	if (status) {
		spin_unlock(&inode->i_lock);
		up(&NFS_MAPSEM(inode));
		dfprintk(PAGECACHE, "nfs_revalidate_inode: (%s/%Ld) refresh failed, error=%d\n",
			 inode->i_sb->s_id,
			 (long long)NFS_FILEID(inode), status);
		goto out;
	}
	cache_validity = nfsi->cache_validity;
	nfsi->cache_validity &= ~NFS_INO_REVAL_PAGECACHE;

	/*
	 * We may need to keep the attributes marked as invalid if
	 * we raced with nfs_end_attr_update().
	 */
	if (verifier == nfsi->cache_change_attribute)
		nfsi->cache_validity &= ~(NFS_INO_INVALID_ATTR|NFS_INO_INVALID_ATIME);
	spin_unlock(&inode->i_lock);
	up(&NFS_MAPSEM(inode));

	nfs_revalidate_mapping(inode, inode->i_mapping);

	if (cache_validity & NFS_INO_INVALID_ACL)
		nfs_zap_acl_cache(inode);

	dfprintk(PAGECACHE, "NFS: (%s/%Ld) revalidation complete\n",
		inode->i_sb->s_id,
		(long long)NFS_FILEID(inode));

	NFS_FLAGS(inode) &= ~NFS_INO_STALE;
out:
	NFS_FLAGS(inode) &= ~NFS_INO_REVALIDATING;
	wake_up(&nfsi->nfs_i_wait);
 out_nowait:
	unlock_kernel();
	return status;
}

int nfs_attribute_timeout(struct inode *inode)
{
	struct nfs_inode *nfsi = NFS_I(inode);

	if (nfs_have_delegation(inode, FMODE_READ))
		return 0;
	return time_after(jiffies, nfsi->read_cache_jiffies+nfsi->attrtimeo);
}

/**
 * nfs_revalidate_inode - Revalidate the inode attributes
 * @server - pointer to nfs_server struct
 * @inode - pointer to inode struct
 *
 * Updates inode attribute information by retrieving the data from the server.
 */
int nfs_revalidate_inode(struct nfs_server *server, struct inode *inode)
{
	if (!(NFS_I(inode)->cache_validity & (NFS_INO_INVALID_ATTR|NFS_INO_INVALID_DATA))
			&& !nfs_attribute_timeout(inode))
		return NFS_STALE(inode) ? -ESTALE : 0;
	return __nfs_revalidate_inode(server, inode);
}

/**
 * nfs_revalidate_mapping - Revalidate the pagecache
 * @inode - pointer to host inode
 * @mapping - pointer to mapping
 */
void nfs_revalidate_mapping(struct inode *inode, struct address_space *mapping)
{
	struct nfs_inode *nfsi = NFS_I(inode);

	if (nfsi->cache_validity & NFS_INO_INVALID_DATA) {
		if (S_ISREG(inode->i_mode)) {
			if (filemap_fdatawrite(mapping) == 0)
				filemap_fdatawait(mapping);
			nfs_wb_all(inode);
		}
		invalidate_inode_pages2(mapping);

		spin_lock(&inode->i_lock);
		nfsi->cache_validity &= ~NFS_INO_INVALID_DATA;
		if (S_ISDIR(inode->i_mode)) {
			memset(nfsi->cookieverf, 0, sizeof(nfsi->cookieverf));
			/* This ensures we revalidate child dentries */
			nfsi->cache_change_attribute++;
		}
		spin_unlock(&inode->i_lock);

		dfprintk(PAGECACHE, "NFS: (%s/%Ld) data cache invalidated\n",
				inode->i_sb->s_id,
				(long long)NFS_FILEID(inode));
	}
}

/**
 * nfs_begin_data_update
 * @inode - pointer to inode
 * Declare that a set of operations will update file data on the server
 */
void nfs_begin_data_update(struct inode *inode)
{
	atomic_inc(&NFS_I(inode)->data_updates);
}

/**
 * nfs_end_data_update
 * @inode - pointer to inode
 * Declare end of the operations that will update file data
 * This will mark the inode as immediately needing revalidation
 * of its attribute cache.
 */
void nfs_end_data_update(struct inode *inode)
{
	struct nfs_inode *nfsi = NFS_I(inode);

	if (!nfs_have_delegation(inode, FMODE_READ)) {
		/* Mark the attribute cache for revalidation */
		spin_lock(&inode->i_lock);
		nfsi->cache_validity |= NFS_INO_INVALID_ATTR;
		/* Directories and symlinks: invalidate page cache too */
		if (S_ISDIR(inode->i_mode) || S_ISLNK(inode->i_mode))
			nfsi->cache_validity |= NFS_INO_INVALID_DATA;
		spin_unlock(&inode->i_lock);
	}
	nfsi->cache_change_attribute ++;
	atomic_dec(&nfsi->data_updates);
}

/**
 * nfs_end_data_update_defer
 * @inode - pointer to inode
 * Declare end of the operations that will update file data
 * This will defer marking the inode as needing revalidation
 * unless there are no other pending updates.
 */
void nfs_end_data_update_defer(struct inode *inode)
{
	struct nfs_inode *nfsi = NFS_I(inode);

	if (atomic_dec_and_test(&nfsi->data_updates)) {
		/* Mark the attribute cache for revalidation */
		nfsi->cache_validity |= NFS_INO_INVALID_ATTR;
		/* Directories and symlinks: invalidate page cache too */
		if (S_ISDIR(inode->i_mode) || S_ISLNK(inode->i_mode))
			nfsi->cache_validity |= NFS_INO_INVALID_DATA;
		nfsi->cache_change_attribute ++;
	}
}

/**
 * nfs_check_inode_attributes - verify consistency of the inode attribute cache
 * @inode - pointer to inode
 * @fattr - updated attributes
 *
 * Verifies the attribute cache. If we have just changed the attributes,
 * so that fattr carries weak cache consistency data, then it may
 * also update the ctime/mtime/change_attribute.
 */
int nfs_refresh_inode(struct inode *inode, struct nfs_fattr *fattr)
{
	struct nfs_inode *nfsi = NFS_I(inode);
	loff_t cur_size, new_isize;
	int data_unstable;

	spin_lock(&inode->i_lock);

	/* Are we in the process of updating data on the server? */
	data_unstable = nfs_caches_unstable(inode);

	if (fattr->valid & NFS_ATTR_FATTR_V4) {
		if ((fattr->valid & NFS_ATTR_PRE_CHANGE) != 0
				&& nfsi->change_attr == fattr->pre_change_attr)
			nfsi->change_attr = fattr->change_attr;
		if (nfsi->change_attr != fattr->change_attr) {
			nfsi->cache_validity |= NFS_INO_INVALID_ATTR;
			if (!data_unstable)
				nfsi->cache_validity |= NFS_INO_REVAL_PAGECACHE;
		}
	}

	if ((fattr->valid & NFS_ATTR_FATTR) == 0) {
		spin_unlock(&inode->i_lock);
		return 0;
	}

	/* Has the inode gone and changed behind our back? */
	if (nfsi->fileid != fattr->fileid
			|| (inode->i_mode & S_IFMT) != (fattr->mode & S_IFMT)) {
		spin_unlock(&inode->i_lock);
		return -EIO;
	}
	
	cur_size = i_size_read(inode);
 	new_isize = nfs_size_to_loff_t(fattr->size);

	/* If we have atomic WCC data, we may update some attributes */
	if ((fattr->valid & NFS_ATTR_WCC) != 0) {
		if (timespec_equal(&inode->i_ctime, &fattr->pre_ctime))
			memcpy(&inode->i_ctime, &fattr->ctime, sizeof(inode->i_ctime));
		if (timespec_equal(&inode->i_mtime, &fattr->pre_mtime))
			memcpy(&inode->i_mtime, &fattr->mtime, sizeof(inode->i_mtime));
	}

	/* Verify a few of the more important attributes */
	if (!timespec_equal(&inode->i_mtime, &fattr->mtime)) {
		nfsi->cache_validity |= NFS_INO_INVALID_ATTR;
		if (!data_unstable)
			nfsi->cache_validity |= NFS_INO_REVAL_PAGECACHE;
	}
	if (cur_size != new_isize) {
		nfsi->cache_validity |= NFS_INO_INVALID_ATTR;
		if (nfsi->npages == 0)
			nfsi->cache_validity |= NFS_INO_REVAL_PAGECACHE;
	}

	/* Have any file permissions changed? */
	if ((inode->i_mode & S_IALLUGO) != (fattr->mode & S_IALLUGO)
			|| inode->i_uid != fattr->uid
			|| inode->i_gid != fattr->gid)
		nfsi->cache_validity |= NFS_INO_INVALID_ATTR | NFS_INO_INVALID_ACCESS | NFS_INO_INVALID_ACL;

	/* Has the link count changed? */
	if (inode->i_nlink != fattr->nlink)
		nfsi->cache_validity |= NFS_INO_INVALID_ATTR;

	if (!timespec_equal(&inode->i_atime, &fattr->atime))
		nfsi->cache_validity |= NFS_INO_INVALID_ATIME;

	nfsi->read_cache_jiffies = fattr->time_start;
	spin_unlock(&inode->i_lock);
	return 0;
}

/**
 * nfs_refresh_inode - try to update the inode attribute cache
 * @inode - pointer to inode
 * @fattr - updated attributes
 *
 * Check that an RPC call that returned attributes has not overlapped with
 * other recent updates of the inode metadata, then decide whether it is
 * safe to do a full update of the inode attributes, or whether just to
 * call nfs_check_inode_attributes.
 */
int nfs_check_inode_attributes(struct inode *inode, struct nfs_fattr *fattr)
{
	struct nfs_inode *nfsi = NFS_I(inode);
	int status;

	if ((fattr->valid & NFS_ATTR_FATTR) == 0)
		return 0;
	down(&NFS_MAPSEM(inode));
	spin_lock(&inode->i_lock);
	nfsi->cache_validity &= ~NFS_INO_REVAL_PAGECACHE;
	if (nfs_verify_change_attribute(inode, fattr->time_start))
		nfsi->cache_validity &= ~(NFS_INO_INVALID_ATTR|NFS_INO_INVALID_ATIME);
	if (time_after(fattr->time_start, nfsi->last_updated))
		status = nfs_update_inode(inode, fattr, fattr->time_start);
	else
		status = nfs_check_inode_attributes(inode, fattr);

	spin_unlock(&inode->i_lock);
	up(&NFS_MAPSEM(inode));
	return status;
}

/*
 * Many nfs protocol calls return the new file attributes after
 * an operation.  Here we update the inode to reflect the state
 * of the server's inode.
 *
 * This is a bit tricky because we have to make sure all dirty pages
 * have been sent off to the server before calling invalidate_inode_pages.
 * To make sure no other process adds more write requests while we try
 * our best to flush them, we make them sleep during the attribute refresh.
 *
 * A very similar scenario holds for the dir cache.
 */
static int nfs_update_inode(struct inode *inode, struct nfs_fattr *fattr, unsigned long verifier)
{
	struct nfs_inode *nfsi = NFS_I(inode);
	__u64		new_size;
	loff_t		new_isize;
	unsigned int	invalid = 0;
	loff_t		cur_isize;
	int data_unstable;

	dfprintk(VFS, "NFS: %s(%s/%ld ct=%d info=0x%x)\n",
			__FUNCTION__, inode->i_sb->s_id, inode->i_ino,
			atomic_read(&inode->i_count), fattr->valid);

	if ((fattr->valid & NFS_ATTR_FATTR) == 0)
		return 0;

	if (nfsi->fileid != fattr->fileid) {
		printk(KERN_ERR "%s: inode number mismatch\n"
		       "expected (%s/0x%Lx), got (%s/0x%Lx)\n",
		       __FUNCTION__,
		       inode->i_sb->s_id, (long long)nfsi->fileid,
		       inode->i_sb->s_id, (long long)fattr->fileid);
		goto out_err;
	}

	/*
	 * Make sure the inode's type hasn't changed.
	 */
	if ((inode->i_mode & S_IFMT) != (fattr->mode & S_IFMT))
		goto out_changed;

	/*
	 * Update the read time so we don't revalidate too often.
	 */
	nfsi->read_cache_jiffies = fattr->time_start;
	nfsi->last_updated = jiffies;

	/* Are we racing with known updates of the metadata on the server? */
	data_unstable = ! (nfs_verify_change_attribute(inode, verifier) ||
		(nfsi->cache_validity & NFS_INO_REVAL_PAGECACHE));

	/* Check if the file size agrees */
	new_size = fattr->size;
 	new_isize = nfs_size_to_loff_t(fattr->size);
	cur_isize = i_size_read(inode);
	if (cur_isize != new_size) {
#ifdef NFS_DEBUG_VERBOSE
		printk(KERN_DEBUG "NFS: isize change on %s/%ld\n", inode->i_sb->s_id, inode->i_ino);
#endif
		/*
		 * If we have pending writebacks, things can get
		 * messy.
		 */
		if (S_ISREG(inode->i_mode) && data_unstable) {
			if (new_isize > cur_isize) {
				inode->i_size = new_isize;
				invalid |= NFS_INO_INVALID_ATTR|NFS_INO_INVALID_DATA;
			}
		} else {
			inode->i_size = new_isize;
			invalid |= NFS_INO_INVALID_ATTR|NFS_INO_INVALID_DATA;
		}
	}

	/*
	 * Note: we don't check inode->i_mtime since pipes etc.
	 *       can change this value in VFS without requiring a
	 *	 cache revalidation.
	 */
	if (!timespec_equal(&inode->i_mtime, &fattr->mtime)) {
		memcpy(&inode->i_mtime, &fattr->mtime, sizeof(inode->i_mtime));
#ifdef NFS_DEBUG_VERBOSE
		printk(KERN_DEBUG "NFS: mtime change on %s/%ld\n", inode->i_sb->s_id, inode->i_ino);
#endif
		if (!data_unstable)
			invalid |= NFS_INO_INVALID_ATTR|NFS_INO_INVALID_DATA;
	}

	if ((fattr->valid & NFS_ATTR_FATTR_V4)
	    && nfsi->change_attr != fattr->change_attr) {
#ifdef NFS_DEBUG_VERBOSE
		printk(KERN_DEBUG "NFS: change_attr change on %s/%ld\n",
		       inode->i_sb->s_id, inode->i_ino);
#endif
		nfsi->change_attr = fattr->change_attr;
		if (!data_unstable)
			invalid |= NFS_INO_INVALID_ATTR|NFS_INO_INVALID_DATA|NFS_INO_INVALID_ACCESS|NFS_INO_INVALID_ACL;
	}

	/* If ctime has changed we should definitely clear access+acl caches */
	if (!timespec_equal(&inode->i_ctime, &fattr->ctime)) {
		if (!data_unstable)
			invalid |= NFS_INO_INVALID_ACCESS|NFS_INO_INVALID_ACL;
		memcpy(&inode->i_ctime, &fattr->ctime, sizeof(inode->i_ctime));
	}
	memcpy(&inode->i_atime, &fattr->atime, sizeof(inode->i_atime));

	if ((inode->i_mode & S_IALLUGO) != (fattr->mode & S_IALLUGO) ||
	    inode->i_uid != fattr->uid ||
	    inode->i_gid != fattr->gid)
		invalid |= NFS_INO_INVALID_ATTR|NFS_INO_INVALID_ACCESS|NFS_INO_INVALID_ACL;

	inode->i_mode = fattr->mode;
	inode->i_nlink = fattr->nlink;
	inode->i_uid = fattr->uid;
	inode->i_gid = fattr->gid;

	if (fattr->valid & (NFS_ATTR_FATTR_V3 | NFS_ATTR_FATTR_V4)) {
		/*
		 * report the blocks in 512byte units
		 */
		inode->i_blocks = nfs_calc_block_size(fattr->du.nfs3.used);
		inode->i_blksize = inode->i_sb->s_blocksize;
 	} else {
 		inode->i_blocks = fattr->du.nfs2.blocks;
 		inode->i_blksize = fattr->du.nfs2.blocksize;
 	}

	/* Update attrtimeo value if we're out of the unstable period */
	if (invalid & NFS_INO_INVALID_ATTR) {
		nfsi->attrtimeo = NFS_MINATTRTIMEO(inode);
		nfsi->attrtimeo_timestamp = jiffies;
	} else if (time_after(jiffies, nfsi->attrtimeo_timestamp+nfsi->attrtimeo)) {
		if ((nfsi->attrtimeo <<= 1) > NFS_MAXATTRTIMEO(inode))
			nfsi->attrtimeo = NFS_MAXATTRTIMEO(inode);
		nfsi->attrtimeo_timestamp = jiffies;
	}
	/* Don't invalidate the data if we were to blame */
	if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode)
				|| S_ISLNK(inode->i_mode)))
		invalid &= ~NFS_INO_INVALID_DATA;
	if (!nfs_have_delegation(inode, FMODE_READ))
		nfsi->cache_validity |= invalid;

	/* qinping mod 20060331 begin */
	if(invalid)
		nfs_zap_caches_locked(inode);
	/* qinping mod 20060331 end */
	return 0;
 out_changed:
	/*
	 * Big trouble! The inode has become a different object.
	 */
#ifdef NFS_PARANOIA
	printk(KERN_DEBUG "%s: inode %ld mode changed, %07o to %07o\n",
			__FUNCTION__, inode->i_ino, inode->i_mode, fattr->mode);
#endif
	/*
	 * No need to worry about unhashing the dentry, as the
	 * lookup validation will know that the inode is bad.
	 * (But we fall through to invalidate the caches.)
	 */
	nfs_invalidate_inode(inode);
 out_err:
	return -EIO;
}

/*
 * File system information
 */

static int nfs_set_super(struct super_block *s, void *data)
{
	s->s_fs_info = data;
	return set_anon_super(s, data);
}
 
static int nfs_compare_super(struct super_block *sb, void *data)
{
	struct nfs_server *server = data;
	struct nfs_server *old = NFS_SB(sb);

	if (old->addr.sin_addr.s_addr != server->addr.sin_addr.s_addr)
		return 0;
	if (old->addr.sin_port != server->addr.sin_port)
		return 0;
	return !nfs_compare_fh(&old->fh, &server->fh);
}

static struct super_block *nfs_get_sb(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *raw_data)
{
	int error;
	struct nfs_server *server;
	struct super_block *s;
	struct nfs_fh *root;
	struct nfs_mount_data *data = raw_data;

	if (!data) {
		/*
		 * XXX: Block devices can not be closed without
		 * 	data->fsuuid. But this case shall be rare.
		 */
		printk("nfs_read_super: missing data argument\n");
		return ERR_PTR(-EINVAL);
	}

	server = kmalloc(sizeof(struct nfs_server), GFP_KERNEL);
	if (!server) {
		vdmap_put_fs_dev(&data->fsuuid);
		return ERR_PTR(-ENOMEM);
	}
	
	memset(server, 0, sizeof(struct nfs_server));
	/* Zero out the NFS state stuff */
	init_nfsv4_state(server);
	server->client = server->client_sys = server->client_acl = ERR_PTR(-EINVAL);

	if (data->version != NFS_MOUNT_VERSION) {
		klog(DEBUG,"nfs warning: mount version %s than kernel\n",
			data->version < NFS_MOUNT_VERSION ? "older" : "newer");
		
		if (data->version < 2)
			data->namlen = 0;
		if (data->version < 3)
			data->bsize  = 0;
		if (data->version < 4) {
			data->flags &= ~NFS_MOUNT_VER3;
			data->root.size = NFS2_FHSIZE;
			memcpy(data->root.data, data->old_root.data, NFS2_FHSIZE);
		}
		if (data->version < 5)
			data->flags &= ~NFS_MOUNT_SECFLAVOUR;
	}

	root = &server->fh;
	if (data->flags & NFS_MOUNT_VER3)
		root->size = data->root.size;
	else
		root->size = NFS2_FHSIZE;
	if (root->size > sizeof(root->data)) {
		printk("nfs_get_sb: invalid root filehandle\n");
		kfree(server);
		vdmap_put_fs_dev(&data->fsuuid);
		return ERR_PTR(-EINVAL);
	}
	memcpy(root->data, data->root.data, root->size);

	/* We now require that the mount process passes the remote address */
	memcpy(&server->addr, &data->addr, sizeof(server->addr));
	if (server->addr.sin_addr.s_addr == INADDR_ANY) {
		printk("NFS: mount program didn't pass remote address!\n");
		kfree(server);
		vdmap_put_fs_dev(&data->fsuuid);
		return ERR_PTR(-EINVAL);
	}

	server->fsuuid = data->fsuuid;

	s = sget(fs_type, nfs_compare_super, nfs_set_super, server);

	if (IS_ERR(s) || s->s_root) {
		kfree(server);
		if (IS_ERR(s))
			vdmap_put_fs_dev(&data->fsuuid);
		return s;
	}

	s->s_flags = flags;

	/* Fire up rpciod if not yet running */
	if (rpciod_up() != 0) {
		printk(KERN_WARNING "NFS: couldn't start rpciod!\n");
		kfree(server);
		vdmap_put_fs_dev(&data->fsuuid);
		return ERR_PTR(-EIO);
	}

	error = nfs_fill_super(s, data, flags & MS_VERBOSE ? 1 : 0);
	if (error) {
		up_write(&s->s_umount);
		deactivate_super(s);
		return ERR_PTR(error);
	}
	s->s_flags |= MS_ACTIVE;
	return s;
}

#ifdef __SUSE9_SUP
static void nfs_kill_super(struct super_block *s)
{
	struct nfs_server *server = NFS_SB(s);
	kill_anon_super(s);
	vdmap_put_fs_dev(&server->fsuuid);
	kfree(server);
}
#else
static void nfs_kill_super(struct super_block *s)
{
	struct nfs_server *server = NFS_SB(s);

	kill_anon_super(s);

	vdmap_put_fs_dev(&server->fsuuid);

	if (server->client != NULL && !IS_ERR(server->client))
		rpc_shutdown_client(server->client);
	if (server->client_sys != NULL && !IS_ERR(server->client_sys))
		rpc_shutdown_client(server->client_sys);
	if (!IS_ERR(server->client_acl))
		rpc_shutdown_client(server->client_acl);

	if (!(server->flags & NFS_MOUNT_NONLM))
	/* qinping mod 20060331 begin */
	{
		//lockd_down();	/* release rpc.lockd */
	}
	/* qinping mod 20060331 end */

	rpciod_down();		/* release rpciod */

	if (server->hostname != NULL)
		kfree(server->hostname);
	kfree(server);
}

#endif

static struct file_system_type nfs_fs_type = {
	.owner		= THIS_MODULE,
	/* qinping mod 20060331 begin */
	//.name		= "nfs", 
	.name		= "enfs", 
	/* qinping mod 20060331 begin */
	.get_sb		= nfs_get_sb,
	.kill_sb	= nfs_kill_super,
	.fs_flags	= FS_ODD_RENAME|FS_REVAL_DOT|FS_BINARY_MOUNTDATA,
};

/* qinping mod 20060331 begin */
/*
 * insert an inode which has pending size notify to the global list.
 * the bflushd will do it after a specific time.
 *
 */
void
inode_attrset_insert(struct inode *inode)
{
	int flag = 0;
	struct nfs_inode *nfsi = NFS_I(inode);

#ifdef ENFS_TEST
	ktlog(TDEBUG1, "attrset_insert: inode = %ld, jiffies = %ld\n", inode->i_ino, jiffies);
	if(!list_empty(&NFS_ATTRSET(inode)))
		klog(DEBUG, "attrset_insert: inode %ld is in the list!\n", inode->i_ino);
#endif

	if(inode_attrset_pid <= 0) {    /* bflushd is gone */
		NFS_FLAGS(inode) |= NFS_INO_SYNC_SIZE;
		NFS_FLAGS(inode) &= ~NFS_INO_SETTING;
		nfsi->npages++;
	}else if(list_empty(&NFS_ATTRSET(inode))){
		spin_lock(&inode_attrset_lock);
		if(list_empty(&NFS_ATTRSET(inode))){
#ifdef ENFS_TEST
			ktlog(TDEBUG1, "attrset_insert: inode %ld isn't in the list!\n", inode->i_ino);
#endif
			if(list_empty(&inode_attrset_list))
				flag = 1;
			list_add_tail(&NFS_ATTRSET(inode), &inode_attrset_list);
			NFS_ATTRSET_TIMEOUT(inode) = jiffies;
			inode_attrset_list_len++;
			NFS_FLAGS(inode) |= NFS_INO_SYNC_SIZE;
			NFS_FLAGS(inode) &= ~NFS_INO_SETTING;
			nfsi->npages++;
			atomic_inc(&inode->i_count); /* hold a reference to the inode. avoid releasing the inode. */
		}
#ifdef ENFS_TEST
		if(!list_empty(&NFS_ATTRSET(inode)))
			ktlog(TDEBUG1, "attrset_insert: inode %ld add into the list!\n\n\n\n", inode->i_ino);
#endif
		spin_unlock(&inode_attrset_lock);
		if(flag && waitqueue_active(&inode_attrset_wait))
			wake_up(&inode_attrset_wait);
	}
}

/* delete an inode size notify from the global list. */
void
inode_attrset_del(struct inode *inode)
{
	struct nfs_inode *nfsi = NFS_I(inode);

#ifdef ENFS_TEST
	ktlog(TDEBUG1, "attrset_del: inode = %ld, jiffies = %ld\n", inode->i_ino, jiffies);
	if(list_empty(&NFS_ATTRSET(inode)))
		ktlog(TDEBUG1, "attrset_del: inode %ld isn't in the list\n", inode->i_ino);
#endif
	if(inode_attrset_pid <= 0){
		NFS_FLAGS(inode) |= NFS_INO_SETTING;
		NFS_FLAGS(inode) &= ~NFS_INO_SYNC_SIZE;
		nfsi->npages = 0;
		if(!list_empty(&NFS_ATTRSET(inode))){
			spin_lock(&inode_attrset_lock);
			if(!list_empty(&NFS_ATTRSET(inode))){
				list_del_init(&NFS_ATTRSET(inode));
				inode_attrset_list_len--;
				atomic_dec(&inode->i_count);
			}
			spin_unlock(&inode_attrset_lock);
		}
	}else if(!list_empty(&NFS_ATTRSET(inode))){
		spin_lock(&inode_attrset_lock);
		if(!list_empty(&NFS_ATTRSET(inode))){
#ifdef ENFS_TEST
			ktlog(TDEBUG1, "attrset_del: inode %ld is in the list\n", inode->i_ino);
#endif
			list_del_init(&NFS_ATTRSET(inode));
			inode_attrset_list_len--;
			NFS_FLAGS(inode) |= NFS_INO_SETTING;
			NFS_FLAGS(inode) &= ~NFS_INO_SYNC_SIZE;
			nfsi->npages = 0;
			atomic_dec(&inode->i_count); /* release the inode reference */
		}
#ifdef ENFS_TEST
		if(list_empty(&NFS_ATTRSET(inode)))
			ktlog(TDEBUG1, "attrset_del: inode %ld is delete from list\n\n\n\n", inode->i_ino);
#endif
		spin_unlock(&inode_attrset_lock);
	}
}

int
inode_attrset_do(struct dentry *dentry, int for_bflushd)
{
	struct iattr attr;
	struct inode *inode = dentry->d_inode;
	int status = 0;

	klog(DEBUG, "inode_attrset_do: inode = %ld for_bflushd = %d\n",
	     inode->i_ino, for_bflushd);
	memset(&attr, 0, sizeof(struct iattr));
	attr.ia_valid |= (ATTR_WRITE | ATTR_MTIME);
	if (for_bflushd)
		attr.ia_valid |= ATTR_BFLUSHD;
	if(NFS_FLAGS(inode) & NFS_INO_SYNC_SIZE){
		down(&inode->i_sem);
		if(NFS_FLAGS(inode) & NFS_INO_SYNC_SIZE){
			inode_attrset_del(inode);
			attr.ia_size = i_size_read(inode);
			status = nfs_setattr(dentry, &attr);
		}
		up(&inode->i_sem);
	}
	return status;
}

int
inode_attrset_do_nolock(struct dentry *dentry)
{
	struct iattr attr;
	struct inode *inode = dentry->d_inode;
	int status = 0;

	klog(DEBUG, "inode_attrset_do_nolock: inode = %ld\n", inode->i_ino);
	memset(&attr, 0, sizeof(struct iattr));
	attr.ia_valid |= (ATTR_WRITE | ATTR_MTIME);
	if(NFS_FLAGS(inode) & NFS_INO_SYNC_SIZE){
		inode_attrset_del(inode);
		attr.ia_size = i_size_read(inode);
		status = nfs_setattr(dentry, &attr);
	}
	return status;
}

static void
inode_attrset_exec(struct inode *inode, int for_bflushd)
{
	struct dentry *dentry;

	klog(DEBUG, "inode_attrset_exec: inode = %ld\n", inode->i_ino);
	dentry = d_find_alias(inode);
	if (!dentry) {
		inode_attrset_del(inode);
		klog(DEBUG, "inode_attrset_exec: failed, inode = %ld\n", inode->i_ino);
		return;
	}
	inode_attrset_do(dentry, for_bflushd);
	dput(dentry);
}

static int
inode_attrset_bflushd(void *unused)
{
	struct list_head *pl;
	struct nfs_inode *enfs_inode;
	int set_count;
	int t;

	lock_kernel();
	daemonize("bflushd");
	/* block all signals eccept SIGKILL */
	allow_signal(SIGKILL);
	unlock_kernel();

	while(1){
cont:
		if(signalled())  /* Is there any SIGKILL */
			break;
		t = ATTRSET_TIMEOUT;
		spin_lock(&inode_attrset_lock);
		if(list_empty(&inode_attrset_list)){
			inode_attrset_list_len = 0;
			spin_unlock(&inode_attrset_lock);
			klog(DEBUG, "inode_attrset_bflushd: empty, jiffies = %ld\n", jiffies);
			wait_event_interruptible(inode_attrset_wait, !list_empty(&inode_attrset_list));
			continue;
		}

		set_count = (inode_attrset_list_len >> 3) + 1;
		klog(DEBUG, "inode_attrset_bflushd: work, list_len = %d, set_count = %d, jiffies = %ld\n",
				 inode_attrset_list_len, set_count, jiffies);
		while(!list_empty(&inode_attrset_list) && !signalled()){
			pl = inode_attrset_list.next;
			enfs_inode = list_entry(pl, struct nfs_inode, attrset);
			t = enfs_inode->attrset_timeout + ATTRSET_TIMEOUT - jiffies;
			if(t > 0) /* the inode has not expired yet. */
				break;
			atomic_inc(&enfs_inode->vfs_inode.i_count);
			spin_unlock(&inode_attrset_lock);
			inode_attrset_exec(&enfs_inode->vfs_inode, 1);
			iput(&enfs_inode->vfs_inode);
			if((--set_count) == 0){
				klog(DEBUG, "inode_attrset_bflushd: schedule, jiffies = %ld\n", jiffies);
				schedule();  /* have a rest. Give chance to others. */
				goto cont;
			}
			spin_lock(&inode_attrset_lock);
		}

		if(signalled()){
			spin_unlock(&inode_attrset_lock);
			klog(DEBUG, "inode_attrset_bflushd: killed\n");
			break;
		}
		if(list_empty(&inode_attrset_list)){
			inode_attrset_list_len = 0;
			spin_unlock(&inode_attrset_lock);
			klog(DEBUG, "inode_attrset_bflushd: empty, sleep = %d, jiffies = %ld\n", 0, jiffies);
			wait_event_interruptible(inode_attrset_wait, !list_empty(&inode_attrset_list));
		}else{
			t = ((t > ATTRSET_MINTIME) ? t : ATTRSET_MINTIME);
			spin_unlock(&inode_attrset_lock);
			klog(DEBUG, "inode_attrset_bflushd: no_empty, sleep = %d, jiffies = %ld\n", t, jiffies);
			wait_event_interruptible_timeout(inode_attrset_wait, 0, t);
		}
	}

	klog(DEBUG, "inode_attrset_bflushd: exit, inode_attrset_pid = %d\n", inode_attrset_pid);
	inode_attrset_pid = 0;  /* I am going to die. wake up the process who kills me. */
	if(waitqueue_active(&inode_attrset_wait))
		wake_up_interruptible(&inode_attrset_wait);
	return 0;
}

static int
inode_attrset_init(void)
{
	int err = 0;

	if(inode_attrset_pid <= 0){
		init_waitqueue_head(&inode_attrset_wait);
		if((inode_attrset_pid = kernel_thread(inode_attrset_bflushd, NULL, 0)) <= 0){
			inode_attrset_pid = 0;
			klog(DEBUG, "inode_attrset_init: cannot start inode_attrset_bflushd!\n");
			err = -1;
		}
	}
	klog(DEBUG, "inode_attrset_init: inode_attrset_pid = %d\n", inode_attrset_pid);
	return err;
}

static void
inode_attrset_exit(void)
{
	if(inode_attrset_pid > 0){
		int i = 50;

		kill_proc(inode_attrset_pid, SIGKILL, 1);
		if(waitqueue_active(&inode_attrset_wait))
			wake_up(&inode_attrset_wait);  /* wake up the bflushd process. */
		while(inode_attrset_pid && (i--)){
			if(waitqueue_active(&inode_attrset_wait))
				wake_up(&inode_attrset_wait);
			/* wait bflusd to exit */
			wait_event_interruptible_timeout(inode_attrset_wait, 0, (HZ >> 1));
		}
	}
	klog(DEBUG, "inode_attrset_exit\n");
}
/* qinping mod 20060331 end */

#ifdef CONFIG_NFS_V4

static void nfs4_clear_inode(struct inode *);


static struct super_operations nfs4_sops = { 
	.alloc_inode	= nfs_alloc_inode,
	.destroy_inode	= nfs_destroy_inode,
	.write_inode	= nfs_write_inode,
	.delete_inode	= nfs_delete_inode,
#ifdef __SUSE9_SUP
	.put_super	= nfs_put_super,
#endif
	.statfs		= nfs_statfs,
	.clear_inode	= nfs4_clear_inode,
	.umount_begin	= nfs_umount_begin,
	.show_options	= nfs_show_options,
};

/*
 * Clean out any remaining NFSv4 state that might be left over due
 * to open() calls that passed nfs_atomic_lookup, but failed to call
 * nfs_open().
 */
static void nfs4_clear_inode(struct inode *inode)
{
	struct nfs_inode *nfsi = NFS_I(inode);

	/* If we are holding a delegation, return it! */
	if (nfsi->delegation != NULL)
		nfs_inode_return_delegation(inode);
	/* First call standard NFS clear_inode() code */
	nfs_clear_inode(inode);
	/* Now clear out any remaining state */
	while (!list_empty(&nfsi->open_states)) {
		struct nfs4_state *state;
		
		state = list_entry(nfsi->open_states.next,
				struct nfs4_state,
				inode_states);
		dprintk("%s(%s/%Ld): found unclaimed NFSv4 state %p\n",
				__FUNCTION__,
				inode->i_sb->s_id,
				(long long)NFS_FILEID(inode),
				state);
		BUG_ON(atomic_read(&state->count) != 1);
		nfs4_close_state(state, state->state);
	}
}


static int nfs4_fill_super(struct super_block *sb, struct nfs4_mount_data *data, int silent)
{
	struct nfs_server *server;
	struct nfs4_client *clp = NULL;
	struct rpc_xprt *xprt = NULL;
	struct rpc_clnt *clnt = NULL;
	struct rpc_timeout timeparms;
	rpc_authflavor_t authflavour;
	int proto, err = -EIO;

	sb->s_blocksize_bits = 0;
	sb->s_blocksize = 0;
	server = NFS_SB(sb);
	if (data->rsize != 0)
		server->rsize = nfs_block_size(data->rsize, NULL);
	if (data->wsize != 0)
		server->wsize = nfs_block_size(data->wsize, NULL);
	server->flags = data->flags & NFS_MOUNT_FLAGMASK;
	server->caps = NFS_CAP_ATOMIC_OPEN;

	server->acregmin = data->acregmin*HZ;
	server->acregmax = data->acregmax*HZ;
	server->acdirmin = data->acdirmin*HZ;
	server->acdirmax = data->acdirmax*HZ;

	server->rpc_ops = &nfs_v4_clientops;
	/* Initialize timeout values */

	timeparms.to_initval = data->timeo * HZ / 10;
	timeparms.to_retries = data->retrans;
	timeparms.to_exponential = 1;
	if (!timeparms.to_retries)
		timeparms.to_retries = 5;

	proto = data->proto;
	/* Which IP protocol do we use? */
	switch (proto) {
	case IPPROTO_TCP:
		timeparms.to_maxval  = RPC_MAX_TCP_TIMEOUT;
		if (!timeparms.to_initval)
			timeparms.to_initval = 600 * HZ / 10;
		break;
	case IPPROTO_UDP:
		timeparms.to_maxval  = RPC_MAX_UDP_TIMEOUT;
		if (!timeparms.to_initval)
			timeparms.to_initval = 11 * HZ / 10;
		break;
	default:
		return -EINVAL;
	}

	clp = nfs4_get_client(&server->addr.sin_addr);
	if (!clp) {
		printk(KERN_WARNING "NFS: failed to create NFS4 client.\n");
		return -EIO;
	}

	/* Now create transport and client */
	authflavour = RPC_AUTH_UNIX;
	if (data->auth_flavourlen != 0) {
		if (data->auth_flavourlen > 1)
			printk(KERN_INFO "NFS: cannot yet deal with multiple auth flavours.\n");
		if (copy_from_user(&authflavour, data->auth_flavours, sizeof(authflavour))) {
			err = -EFAULT;
			goto out_fail;
		}
	}

	down_write(&clp->cl_sem);
	if (clp->cl_rpcclient == NULL) {
		xprt = xprt_create_proto(proto, &server->addr, &timeparms);
		if (IS_ERR(xprt)) {
			up_write(&clp->cl_sem);
			printk(KERN_WARNING "NFS: cannot create RPC transport.\n");
			err = PTR_ERR(xprt);
			goto out_fail;
		}
		clnt = rpc_create_client(xprt, server->hostname, &nfs_program,
				server->rpc_ops->version, authflavour);
		if (IS_ERR(clnt)) {
			up_write(&clp->cl_sem);
			printk(KERN_WARNING "NFS: cannot create RPC client.\n");
			xprt_destroy(xprt);
			err = PTR_ERR(clnt);
			goto out_fail;
		}
		clnt->cl_chatty   = 1;
		clp->cl_rpcclient = clnt;
		clp->cl_cred = rpcauth_lookupcred(clnt->cl_auth, 0);
		memcpy(clp->cl_ipaddr, server->ip_addr, sizeof(clp->cl_ipaddr));
		nfs_idmap_new(clp);
	}
	if (list_empty(&clp->cl_superblocks)) {
		err = nfs4_init_client(clp);
		if (err != 0) {
			up_write(&clp->cl_sem);
			goto out_fail;
		}
	}
	list_add_tail(&server->nfs4_siblings, &clp->cl_superblocks);
	clnt = rpc_clone_client(clp->cl_rpcclient);
	if (!IS_ERR(clnt))
			server->nfs4_state = clp;
	up_write(&clp->cl_sem);
	clp = NULL;

	if (IS_ERR(clnt)) {
		printk(KERN_WARNING "NFS: cannot create RPC client.\n");
		return PTR_ERR(clnt);
	}

	clnt->cl_intr     = (server->flags & NFS4_MOUNT_INTR) ? 1 : 0;
	clnt->cl_softrtry = (server->flags & NFS4_MOUNT_SOFT) ? 1 : 0;
	server->client    = clnt;

	if (server->nfs4_state->cl_idmap == NULL) {
		printk(KERN_WARNING "NFS: failed to create idmapper.\n");
		return -ENOMEM;
	}

	if (clnt->cl_auth->au_flavor != authflavour) {
		if (rpcauth_create(authflavour, clnt) == NULL) {
			printk(KERN_WARNING "NFS: couldn't create credcache!\n");
			return -ENOMEM;
		}
	}

	sb->s_op = &nfs4_sops;
	err = nfs_sb_init(sb, authflavour);
	if (err == 0)
		return 0;
out_fail:
	if (clp)
		nfs4_put_client(clp);
	return err;
}

static int nfs4_compare_super(struct super_block *sb, void *data)
{
	struct nfs_server *server = data;
	struct nfs_server *old = NFS_SB(sb);

	if (strcmp(server->hostname, old->hostname) != 0)
		return 0;
	if (strcmp(server->mnt_path, old->mnt_path) != 0)
		return 0;
	return 1;
}

static void *
nfs_copy_user_string(char *dst, struct nfs_string *src, int maxlen)
{
	void *p = NULL;

	if (!src->len)
		return ERR_PTR(-EINVAL);
	if (src->len < maxlen)
		maxlen = src->len;
	if (dst == NULL) {
		p = dst = kmalloc(maxlen + 1, GFP_KERNEL);
		if (p == NULL)
			return ERR_PTR(-ENOMEM);
	}
	if (copy_from_user(dst, src->data, maxlen)) {
		if (p != NULL)
			kfree(p);
		return ERR_PTR(-EFAULT);
	}
	dst[maxlen] = '\0';
	return dst;
}

static struct super_block *nfs4_get_sb(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *raw_data)
{
	int error;
	struct nfs_server *server;
	struct super_block *s;
	struct nfs4_mount_data *data = raw_data;
	void *p;

	if (!data) {
		printk("nfs_read_super: missing data argument\n");
		return ERR_PTR(-EINVAL);
	}

	server = kmalloc(sizeof(struct nfs_server), GFP_KERNEL);
	if (!server)
		return ERR_PTR(-ENOMEM);
	memset(server, 0, sizeof(struct nfs_server));
	/* Zero out the NFS state stuff */
	init_nfsv4_state(server);
	server->client = server->client_sys = server->client_acl = ERR_PTR(-EINVAL);

	if (data->version != NFS4_MOUNT_VERSION) {
		printk("nfs warning: mount version %s than kernel\n",
			data->version < NFS4_MOUNT_VERSION ? "older" : "newer");
	}

	p = nfs_copy_user_string(NULL, &data->hostname, 256);
	if (IS_ERR(p))
		goto out_err;
	server->hostname = p;

	p = nfs_copy_user_string(NULL, &data->mnt_path, 1024);
	if (IS_ERR(p))
		goto out_err;
	server->mnt_path = p;

	p = nfs_copy_user_string(server->ip_addr, &data->client_addr,
			sizeof(server->ip_addr) - 1);
	if (IS_ERR(p))
		goto out_err;

	/* We now require that the mount process passes the remote address */
	if (data->host_addrlen != sizeof(server->addr)) {
		s = ERR_PTR(-EINVAL);
		goto out_free;
	}
	if (copy_from_user(&server->addr, data->host_addr, sizeof(server->addr))) {
		s = ERR_PTR(-EFAULT);
		goto out_free;
	}
	if (server->addr.sin_family != AF_INET ||
	    server->addr.sin_addr.s_addr == INADDR_ANY) {
		printk("NFS: mount program didn't pass remote IP address!\n");
		s = ERR_PTR(-EINVAL);
		goto out_free;
	}

	s = sget(fs_type, nfs4_compare_super, nfs_set_super, server);

	if (IS_ERR(s) || s->s_root)
		goto out_free;

	s->s_flags = flags;

	/* Fire up rpciod if not yet running */
	if (rpciod_up() != 0) {
		printk(KERN_WARNING "NFS: couldn't start rpciod!\n");
		s = ERR_PTR(-EIO);
		goto out_free;
	}

	error = nfs4_fill_super(s, data, flags & MS_VERBOSE ? 1 : 0);
	if (error) {
		up_write(&s->s_umount);
		deactivate_super(s);
		return ERR_PTR(error);
	}
	s->s_flags |= MS_ACTIVE;
	return s;
out_err:
	s = (struct super_block *)p;
out_free:
	if (server->mnt_path)
		kfree(server->mnt_path);
	if (server->hostname)
		kfree(server->hostname);
	kfree(server);
	return s;
}

static void nfs4_kill_super(struct super_block *sb)
{
	struct nfs_server *server = NFS_SB(sb);

	nfs_return_all_delegations(sb);
	kill_anon_super(sb);

	nfs4_renewd_prepare_shutdown(server);

	if (server->client != NULL && !IS_ERR(server->client))
		rpc_shutdown_client(server->client);
	rpciod_down();		/* release rpciod */

	destroy_nfsv4_state(server);

	if (server->hostname != NULL)
		kfree(server->hostname);
	kfree(server);
}

static struct file_system_type nfs4_fs_type = {
	.owner		= THIS_MODULE,
	/* qinping mod 20060331 begin */
	//.name		= "nfs4",
	.name		= "enfs4",
	/* qinping mod 20060331 end */
	.get_sb		= nfs4_get_sb,
	.kill_sb	= nfs4_kill_super,
	.fs_flags	= FS_ODD_RENAME|FS_REVAL_DOT|FS_BINARY_MOUNTDATA,
};

#define nfs4_init_once(nfsi) \
	do { \
		INIT_LIST_HEAD(&(nfsi)->open_states); \
		nfsi->delegation = NULL; \
		nfsi->delegation_state = 0; \
		init_rwsem(&nfsi->rwsem); \
	} while(0)
#define register_nfs4fs() register_filesystem(&nfs4_fs_type)
#define unregister_nfs4fs() unregister_filesystem(&nfs4_fs_type)
#else
#define nfs4_init_once(nfsi) \
	do { } while (0)
#define register_nfs4fs() (0)
#define unregister_nfs4fs()
#endif

extern int nfs_init_nfspagecache(void);
extern void nfs_destroy_nfspagecache(void);
extern int nfs_init_readpagecache(void);
extern void nfs_destroy_readpagecache(void);
extern int nfs_init_writepagecache(void);
extern void nfs_destroy_writepagecache(void);
#ifdef CONFIG_NFS_DIRECTIO
extern int nfs_init_directcache(void);
extern void nfs_destroy_directcache(void);
#endif

static kmem_cache_t * nfs_inode_cachep;

static struct inode *nfs_alloc_inode(struct super_block *sb)
{
	struct nfs_inode *nfsi;
	nfsi = (struct nfs_inode *)kmem_cache_alloc(nfs_inode_cachep, SLAB_KERNEL);
	if (!nfsi)
		return NULL;
	nfsi->flags = 0UL;
	nfsi->cache_validity = 0UL;
#ifdef __SUSE9_SUP
#ifdef CONFIG_NFS_ACL
	nfsi->acl_access = ERR_PTR(-EAGAIN);
	nfsi->acl_default = ERR_PTR(-EAGAIN);
#endif
#else
#ifdef CONFIG_NFS_V3_ACL
	nfsi->acl_access = ERR_PTR(-EAGAIN);
	nfsi->acl_default = ERR_PTR(-EAGAIN);
#endif
#endif /*__SUSE9_SUP*/
	return &nfsi->vfs_inode;
}

static void nfs_destroy_inode(struct inode *inode)
{
	kmem_cache_free(nfs_inode_cachep, NFS_I(inode));
}

static void init_once(void * foo, kmem_cache_t * cachep, unsigned long flags)
{
	struct nfs_inode *nfsi = (struct nfs_inode *) foo;

	if ((flags & (SLAB_CTOR_VERIFY|SLAB_CTOR_CONSTRUCTOR)) ==
	    SLAB_CTOR_CONSTRUCTOR) {
		inode_init_once(&nfsi->vfs_inode);
		spin_lock_init(&nfsi->req_lock);
		INIT_LIST_HEAD(&nfsi->dirty);
		INIT_LIST_HEAD(&nfsi->commit);
		INIT_LIST_HEAD(&nfsi->open_files);
		INIT_RADIX_TREE(&nfsi->nfs_page_tree, GFP_ATOMIC);
		atomic_set(&nfsi->data_updates, 0);
		nfsi->ndirty = 0;
		nfsi->ncommit = 0;
		nfsi->npages = 0;
		init_waitqueue_head(&nfsi->nfs_i_wait);
		nfs4_init_once(nfsi);
	}
}
 
int nfs_init_inodecache(void)
{
	nfs_inode_cachep = kmem_cache_create("enfs_inode_cache",
					     sizeof(struct nfs_inode),
					     0, SLAB_RECLAIM_ACCOUNT,
					     init_once, NULL);
	if (nfs_inode_cachep == NULL)
		return -ENOMEM;

	return 0;
}

void nfs_destroy_inodecache(void)
{
	if (kmem_cache_destroy(nfs_inode_cachep))
		printk(KERN_INFO "nfs_inode_cache: not all structures were freed\n");
}

#ifdef CONFIG_PROC_FS

/*
 * The proc interface
 */

#include <linux/file.h>

static struct proc_dir_entry *enfs_proc_root;

/*zdz add 20061205 begin*/
static int rapage_read_proc(char *page, char **start, off_t off, int count,
			    int *eof, void *data)
{
	int ret;
	ret = sprintf(page,"%d", *(int *)data);
	*eof = 1;
	return ret;
}

static int rapage_write_proc(struct file *file, const char __user *buffer,
			     unsigned long count, void *data)
{
	char buf[32];

        if (count > ARRAY_SIZE(buf) - 1)
                count = ARRAY_SIZE(buf) - 1;
        if (copy_from_user(buf, buffer, count))
                return -EFAULT;
        buf[ARRAY_SIZE(buf) - 1] = '\0';
        enfs_ra_pages = simple_strtoul(buf, NULL, 10);
	if (enfs_ra_pages > MAXRAPAGES) enfs_ra_pages = MAXRAPAGES;
        if (enfs_ra_pages < MINRAPAGES) enfs_ra_pages = MINRAPAGES;
	if(pserver!=NULL){
		if(enfs_ra_flag)
        		pserver->backing_dev_info.ra_pages = enfs_ra_pages; 
		else
        		pserver->backing_dev_info.ra_pages = pserver->rpages * NFS_MAX_READAHEAD;
	}

        return count;
}

static int raflag_read_proc(char *page, char **start, off_t off, int count,
			    int *eof, void *data)
{
        int ret;
        ret = sprintf(page,"%d", *(int *)data);
        *eof = 1;
        return ret;
}

static int raflag_write_proc(struct file *file, const char __user *buffer,
			     unsigned long count, void *data)
{
        char buf[32];

        if (count > ARRAY_SIZE(buf) - 1)
                count = ARRAY_SIZE(buf) - 1;
        if (copy_from_user(buf, buffer, count))
                return -EFAULT;
        buf[ARRAY_SIZE(buf) - 1] = '\0';
        enfs_ra_flag = (simple_strtol(buf, NULL, 10)?1:0);
	
        return count;
}
/*zdz add 20061205 end*/

#ifdef ENFS_TEST

static int read_tdebug(char *page, char **start, off_t off, int count,
		       int *eof, void *data)
{
	int ret;

	ret = sprintf(page + off, "%d\n", t_debug_level);
	*eof = 1;
	return ret;
}

static int write_tdebug(struct file *file, const char *buffer,
			unsigned long count, void *data)
{
	char buf[32];

	if (count > ARRAY_SIZE(buf) - 1)
		count = ARRAY_SIZE(buf) - 1;
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;
	buf[ARRAY_SIZE(buf) - 1] = '\0';
	t_debug_level = simple_strtoul(buf, NULL, 10);
	if (t_debug_level > TDEBUG4)  t_debug_level = TDEBUG4;
	if (t_debug_level < TDEBUG1)  t_debug_level = TDEBUG1;
	return count;
}

static int bcache_read(char *buf, char **start, off_t offs, int len, int *eof,
		       void *data)
{
	*eof = 1;
	return snprintf(buf, len,
			"stat = %d\n",
			bcache_stat.flag);
}

static int bcache_write(struct file *file, const char *buffer,
			unsigned long len, void *data)
{
	char buf[10];
	if(copy_from_user(buf, buffer, 10)){
		return -EFAULT;
	}
	sscanf(buffer, "%d", &bcache_stat.flag);
	return len;
}

#define ATTRSETLIST	"enfsattrsetlist"
#define BFLUSHDPID	"enfsbflushdpid"
#define SAERROR		"enfssaerror"

static int bflushdpid_read_proc(char *page, char **start, off_t offset,
                                int count, int *eof, void *data)
{
	pid_t pid;
	int n;

	pid = inode_attrset_pid;
	/* A page should be large enough for the string
	   representation of a PID. */
	n = sprintf(page, "%lld\n", (long long) pid);
	/* Only complete reads are supported. */
	if (n > count)
		return -EINVAL;
	*eof = 1;
	return n;
}

static int saerror_read_proc(char *page, char **start, off_t offset, int count,
                             int *eof, void *data)
{
	int i, n;

	i = atomic_read(&enfs_saerror);
	n = sprintf(page, "%d\n", i);
	if (n > count)
		return -EINVAL;
	*eof = 1;
	return n;
}

static int saerror_write_proc(struct file *f, const char __user *buf,
                              unsigned long count, void *data)
{
	char s[2];
	unsigned int i;

	if ((count < 1) || (f->f_pos != 0))
		return -EINVAL;
	if (copy_from_user(s, buf, 1))
		return -EFAULT;
	s[1] = '\0';
	if (sscanf(s, "%u", &i) != 1)
		return -EINVAL;
	atomic_set(&enfs_saerror, i);
	return count;
}

static int attrsetlist_seq_show(struct seq_file *sf, void *v)
{
	struct nfs_inode *nfsi;
	struct list_head *e;

	spin_lock(&inode_attrset_lock);
	seq_printf(sf, "%d\n", inode_attrset_list_len);
	list_for_each(e, &inode_attrset_list) {
		nfsi = list_entry(e, struct nfs_inode, attrset);
		seq_printf(sf, "%lu\n", nfsi->vfs_inode.i_ino);
	}
	spin_unlock(&inode_attrset_lock);
	return 0;
}

static int attrsetlist_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, attrsetlist_seq_show, NULL);
}

static struct file_operations attrsetlist_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= attrsetlist_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release
};

static int attrset_proc_init(struct proc_dir_entry *dir)
{
	struct proc_dir_entry *e;

	e = create_proc_entry(ATTRSETLIST, 0666, dir);
	if (!e)
		goto err0;
	e->proc_fops = &attrsetlist_proc_fops;

	e = create_proc_read_entry(BFLUSHDPID, 0666, dir,
	                           bflushdpid_read_proc, NULL);
	if (!e)
		goto err1;

	e = create_proc_entry(SAERROR, 0666, dir);
	if (!e)
		goto err2;
	e->read_proc = saerror_read_proc;
	e->write_proc = saerror_write_proc;

	return 0;

err2:
	remove_proc_entry(BFLUSHDPID, dir);
err1:
	remove_proc_entry(ATTRSETLIST, dir);
err0:
	return -ENOMEM;
}

static void attrset_proc_destroy(struct proc_dir_entry *dir)
{
	remove_proc_entry(SAERROR, dir);
	remove_proc_entry(BFLUSHDPID, dir);
	remove_proc_entry(ATTRSETLIST, dir);
}

#endif	/* ENFS_TEST */

static int enfs_init_proc(void)
{
	struct proc_dir_entry *p;

	enfs_proc_root = proc_mkdir("enfs", proc_root_fs);
	if (!enfs_proc_root)
		goto err0;

	/* qinping mod 20060331 begin */
#ifdef ENFS_TEST
	p = create_proc_entry("tdebug", 0666, enfs_proc_root);
	if (!p)
		goto err1;
	p->read_proc = read_tdebug;
	p->write_proc = write_tdebug;

	p = create_proc_entry("my_bcache_stat", 0666, enfs_proc_root);
	if (!p)
		goto err2;
	p->read_proc = bcache_read;
	p->write_proc = bcache_write;

	if (attrset_proc_init(enfs_proc_root))
		goto err3;
#endif
	/* qinping mod 20060331 end */

	/*zdz add 20061205 begin*/
	p = create_proc_entry("ra_pages", 0644, enfs_proc_root);
	if (!p)
		goto err4;
	p->data = &enfs_ra_pages;
	p->read_proc = rapage_read_proc;
	p->write_proc = rapage_write_proc;

	p = create_proc_entry("ra_flag", 0644, enfs_proc_root);
	if (!p)
		goto err5;
	p->data = &enfs_ra_flag;
	p->read_proc = raflag_read_proc;
	p->write_proc = raflag_write_proc;
	/*zdz add 20061205 end*/	

	/* We shall re-enable this one day, after the line between
	   NFS and ENFS is clearly drawn. */
#if 0
	p = rpc_proc_register(&nfs_rpcstat);
	if (!p)
		goto err6;
#endif

	return 0;

#if 0
err6:
	remove_proc_entry("ra_flag", enfs_proc_root);
#endif
err5:
	remove_proc_entry("ra_pages", enfs_proc_root);
err4:
#ifdef ENFS_TEST
	attrset_proc_destroy(enfs_proc_root);
err3:
	remove_proc_entry("my_bcache_stat", enfs_proc_root);
err2:
	remove_proc_entry("tdebug", enfs_proc_root);
err1:
#endif
	remove_proc_entry("enfs", proc_root_fs);
err0:
	return -ENOMEM;
}

static void enfs_destroy_proc(void)
{
#if 0
	rpc_proc_unregister(nfs_rpcstat.program.name);
#endif
	remove_proc_entry("ra_flag", enfs_proc_root);
	remove_proc_entry("ra_pages", enfs_proc_root);
#ifdef ENFS_TEST
	attrset_proc_destroy(enfs_proc_root);
	remove_proc_entry("my_bcache_stat", enfs_proc_root);
	remove_proc_entry("tdebug", enfs_proc_root);
#endif
	remove_proc_entry("enfs", proc_root_fs);
}

#endif	/* CONFIG_PROC_FS */

static int __init init_nfs_fs(void)
{
	int err;

	err = nfs_init_nfspagecache();
	if (err)
		goto out0;

	err = nfs_init_inodecache();
	if (err)
		goto out1;

	err = nfs_init_readpagecache();
	if (err)
		goto out2;

	err = nfs_init_writepagecache();
	if (err)
		goto out3;

#ifdef CONFIG_NFS_DIRECTIO
	err = nfs_init_directcache();
	if (err)
		goto out4;
#endif

	/* qinping mod 20060331 begin */
	err = inode_attrset_init();
	if (err)
		goto out5;
	/* qinping mod 20060331 end */

#ifdef CONFIG_PROC_FS
	err = enfs_init_proc();
	if (err)
		goto out6;
#endif

	err = register_filesystem(&nfs_fs_type);
	if (err)
		goto out7;

	err = register_nfs4fs();
	if (err)
		goto out8;

	return 0;

out8:
	unregister_filesystem(&nfs_fs_type);
out7:
#ifdef CONFIG_PROC_FS
	enfs_destroy_proc();
out6:
#endif
	inode_attrset_exit();
out5:
#ifdef CONFIG_NFS_DIRECTIO
	nfs_destroy_directcache();
out4:
#endif
	nfs_destroy_writepagecache();
out3:
	nfs_destroy_readpagecache();
out2:
	nfs_destroy_inodecache();
out1:
	nfs_destroy_nfspagecache();
out0:
	return err;
}

static void __exit exit_nfs_fs(void)
{
	unregister_nfs4fs();
	unregister_filesystem(&nfs_fs_type);
#ifdef CONFIG_PROC_FS
	enfs_destroy_proc();
#endif
	/* qinping mod 20060331 begin */
	inode_attrset_exit();
	/* qinping mod 20060331 end */
#ifdef CONFIG_NFS_DIRECTIO
	nfs_destroy_directcache();
#endif
	nfs_destroy_writepagecache();
	nfs_destroy_readpagecache();
	nfs_destroy_inodecache();
	nfs_destroy_nfspagecache();
}

MODULE_AUTHOR("NRCHPC");
MODULE_LICENSE("GPL");

module_init(init_nfs_fs)
module_exit(exit_nfs_fs)
