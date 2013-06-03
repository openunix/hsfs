/*
 * Copyright (C) 2012 Feng Shuo <steve.shuo.feng@gmail.com>
 *
 * This file is part of HSFS and based on linux/fs/nfs/inode.c
 *
 * HSFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * HSFS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HSFS.  If not, see <http://www.gnu.org/licenses/>.
 */

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
#ifndef __KERNEL__

#include <hsfs/err.h>
#include <hsfs/hsi_nfs.h>

#define S_IRWXUGO (S_IRWXU|S_IRWXG|S_IRWXO)
#define S_IALLUGO (S_ISUID|S_ISGID|S_ISVTX|S_IRWXUGO)
#define S_IRUGO (S_IRUSR|S_IRGRP|S_IROTH)
#define S_IWUGO (S_IWUSR|S_IWGRP|S_IWOTH)
#define S_IXUGO (S_IXUSR|S_IXGRP|S_IXOTH)

#else

#include <linux/module.h>
#include <linux/init.h>

#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/sunrpc/clnt.h>
#include <linux/sunrpc/stats.h>
#include <linux/sunrpc/metrics.h>
#include <linux/nfs_fs.h>
#include <linux/nfs_mount.h>
#include <linux/nfs4_mount.h>
#include <linux/lockd/bind.h>
#include <linux/smp_lock.h>
#include <linux/seq_file.h>
#include <linux/mount.h>
#include <linux/nfs_idmap.h>
#include <linux/vfs.h>
#include <linux/inet.h>
#include <linux/nfs_xdr.h>

#include <asm/system.h>
#include <asm/uaccess.h>

#include "nfs4_fs.h"
#include "callback.h"
#include "delegation.h"
#include "iostat.h"
#include "internal.h"

#define NFSDBG_FACILITY		NFSDBG_VFS
#define NFS_PARANOIA 1

#define NFS_64_BIT_INODE_NUMBERS_ENABLED       1

/* Default is to see 64-bit inode numbers */
static int enable_ino64 = NFS_64_BIT_INODE_NUMBERS_ENABLED;

static void nfs_invalidate_inode(struct inode *);
#endif
static int nfs_update_inode(struct hsfs_inode *, struct nfs_fattr *);
#if 0
static void nfs_zap_acl_cache(struct inode *);

static kmem_cache_t * nfs_inode_cachep;

/**
 * nfs_compat_user_ino64 - returns the user-visible inode number
 * @fileid: 64-bit fileid
 *
 * This function returns a 32-bit inode number if the boot parameter
 * nfs.enable_ino64 is zero.
 */
u64 nfs_compat_user_ino64(u64 fileid)
{
	int ino;

	if (enable_ino64)
		return fileid;
	ino = fileid;
	if (sizeof(ino) < sizeof(fileid))
		ino ^= fileid >> (sizeof(fileid)-sizeof(ino)) * 8;
	return ino;
}

int nfs_write_inode(struct inode *inode, int sync)
{
	int flags = sync ? FLUSH_SYNC : 0;
	int ret;

	ret = nfs_commit_inode(inode, flags);
	if (ret < 0)
		return ret;
	return 0;
}

void nfs_clear_inode(struct inode *inode)
{
	/*
	 * The following should never happen...
	 */
	BUG_ON(nfs_have_writebacks(inode));
	BUG_ON (!list_empty(&NFS_I(inode)->open_files));
	nfs_zap_acl_cache(inode);
	nfs_access_zap_cache(inode);

	nfs_fscache_release_cookie(inode);
}

/**
 * nfs_sync_mapping - helper to flush all mmapped dirty data to disk
 */
int nfs_sync_mapping(struct address_space *mapping)
{
	int ret;

	if (mapping->nrpages == 0)
		return 0;
	unmap_mapping_range(mapping, 0, 0, 0);
	ret = filemap_write_and_wait(mapping);
	if (ret != 0)
		goto out;
	ret = nfs_wb_all(mapping->host);
out:
	return ret;
}

/*
 * Invalidate the local caches
 */
static void nfs_zap_caches_locked(struct inode *inode)
{
	struct nfs_inode *nfsi = NFS_I(inode);
	int mode = inode->i_mode;

	nfs_inc_stats(inode, NFSIOS_ATTRINVALIDATE);

	NFS_ATTRTIMEO(inode) = NFS_MINATTRTIMEO(inode);
	NFS_ATTRTIMEO_UPDATE(inode) = jiffies;

	memset(NFS_COOKIEVERF(inode), 0, sizeof(NFS_COOKIEVERF(inode)));
	if (S_ISREG(mode) || S_ISDIR(mode) || S_ISLNK(mode))
		nfsi->cache_validity |= NFS_INO_INVALID_ATTR|NFS_INO_INVALID_DATA|NFS_INO_INVALID_ACCESS|NFS_INO_INVALID_ACL|NFS_INO_REVAL_PAGECACHE;
	else
		nfsi->cache_validity |= NFS_INO_INVALID_ATTR|NFS_INO_INVALID_ACCESS|NFS_INO_INVALID_ACL|NFS_INO_REVAL_PAGECACHE;
}

void nfs_zap_caches(struct inode *inode)
{
	spin_lock(&inode->i_lock);
	nfs_zap_caches_locked(inode);
	spin_unlock(&inode->i_lock);

	nfs_fscache_zap_cookie(inode);
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

void nfs_invalidate_atime(struct inode *inode)
{
	spin_lock(&inode->i_lock);
	NFS_I(inode)->cache_validity |= NFS_INO_INVALID_ATIME;
	spin_unlock(&inode->i_lock);
}

/*
 * Invalidate, but do not unhash, the inode.
 * NB: must be called with inode->i_lock held!
 */
static void nfs_invalidate_inode(struct inode *inode)
{
	set_bit(NFS_INO_STALE, &NFS_FLAGS(inode));
	nfs_zap_caches_locked(inode);
}
#endif	/* __UNUSED__ */

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
nfs_find_actor(struct hsfs_inode *inode, void *opaque)
{
	struct nfs_find_desc	*desc = (struct nfs_find_desc *)opaque;
	struct nfs_fh		*fh = desc->fh;
	struct nfs_fattr	*fattr = desc->fattr;

	if (NFS_FILEID(inode) != fattr->fileid)
		return 0;
	if (nfs_compare_fh(NFS_FH(inode), fh))
		return 0;
	if (is_bad_inode(inode) || NFS_STALE(inode))
		return 0;

	DEBUG("NFS Find HSFS Inode(%p) with (ID:%llu, FH:%p)", inode,
	      (unsigned long long)fattr->fileid, desc->fh);
	return 1;
}

static int
nfs_init_locked(struct hsfs_inode *inode, void *opaque)
{
	struct nfs_find_desc	*desc = (struct nfs_find_desc *)opaque;
	struct nfs_fattr	*fattr = desc->fattr;

	set_nfs_fileid(inode, fattr->fileid);
	nfs_copy_fh(NFS_FH(inode), desc->fh);

	DEBUG("NFS Init HSFS Inode(%p) with (ID:%llu, FH:%p)", inode, 
	      (unsigned long long)fattr->fileid, desc->fh);
	return 0;
}

/* Don't use READDIRPLUS on directories that we believe are too large */
#define NFS_LIMIT_READDIRPLUS (8*PAGE_SIZE)

struct hsfs_inode *nfs_alloc_inode(struct hsfs_super *sb _U_)
{
	struct nfs_inode *nfsi;

	nfsi = (struct nfs_inode *)malloc(sizeof(struct nfs_inode));
	if (!nfsi)
		return NULL;
	bzero(nfsi, sizeof(struct nfs_inode));

#ifdef CONFIG_NFS_V3_ACL
	nfsi->acl_access = ERR_PTR(-EAGAIN);
	nfsi->acl_default = ERR_PTR(-EAGAIN);
#endif
#ifdef CONFIG_NFS_V4
	nfsi->nfs4_acl = NULL;
#endif /* CONFIG_NFS_V4 */
	
	DEBUG("Alloc NFS Inode at %p, HSFS at %p", nfsi, &nfsi->hsfs_inode);
	return &nfsi->hsfs_inode;
}

void nfs_destroy_inode(struct hsfs_inode *inode)
{
	free(NFS_I(inode));
}

/*
 * This is our front-end to iget that looks up inodes by file handle
 * instead of inode number.
 */
struct hsfs_inode *
hsi_nfs_fhget(struct hsfs_super *sb, struct nfs_fh *fh, struct nfs_fattr *fattr)
{
	struct nfs_find_desc desc = {
		.fh	= fh,
		.fattr	= fattr
	};
	struct hsfs_inode *inode = ERR_PTR(-ENOENT);
	uint64_t hash;

	DEBUG_IN("(%p, %p, %p)", sb, fh, fattr);
	
	if ((fattr->valid & NFS_ATTR_FATTR) == 0)
		goto out_no_inode;

	if (!fattr->nlink) {
		ERR("NFS: Buggy server - nlink == 0!\n");
		goto out_no_inode;
	}

	hash = fattr->fileid;

	inode = hsfs_iget5_locked(sb, hash, nfs_find_actor, nfs_init_locked, &desc);
	if (inode == NULL) {
		inode = ERR_PTR(-ENOMEM);
		goto out_no_inode;
	}

	if (inode->i_state & I_NEW) {
		struct nfs_inode *nfsi _U_ = NFS_I(inode);

		/* We set i_ino for the few things that still rely on it, such
		 * as printing messages; stat and filldir use the fileid
		 * directly since i_ino may not be large enough */
		set_nfs_fileid(inode, fattr->fileid);

		/* We can't support update_atime(), since the server will reset it */
//		inode->i_flags |= S_NOATIME|S_NOCMTIME|S_NOATTRKILL;
		inode->i_mode = fattr->mode;
#if __UNUSED__
		/* Why so? Because we want revalidate for devices/FIFOs, and
		 * that's precisely what we have in nfs_file_inode_operations.
		 */
		inode->i_op = NFS_SB(sb)->nfs_client->rpc_ops->file_inode_ops;
		if (S_ISREG(inode->i_mode)) {
			inode->i_fop = &nfs_file_operations;
			inode->i_data.a_ops = &nfs_file_aops;
			inode->i_data.backing_dev_info = &NFS_SB(sb)->backing_dev_info;
		} else if (S_ISDIR(inode->i_mode)) {
			inode->i_op = NFS_SB(sb)->nfs_client->rpc_ops->dir_inode_ops;
			inode->i_fop = &nfs_dir_operations;
			if (nfs_server_capable(inode, NFS_CAP_READDIRPLUS)
			    && fattr->size <= NFS_LIMIT_READDIRPLUS)
				set_bit(NFS_INO_ADVISE_RDPLUS, &NFS_FLAGS(inode));
			/* Deal with crossing mountpoints */
			if (!nfs_fsid_equal(&NFS_SB(sb)->fsid, &fattr->fsid)) {
				if (fattr->valid & NFS_ATTR_FATTR_V4_REFERRAL)
					inode->i_op = &nfs_referral_inode_operations;
				else
					inode->i_op = &nfs_mountpoint_inode_operations;
				inode->i_fop = NULL;
				set_bit(NFS_INO_MOUNTPOINT, &nfsi->flags);
			}
		} else if (S_ISLNK(inode->i_mode))
			inode->i_op = &nfs_symlink_inode_operations;
		else
			init_special_inode(inode, inode->i_mode, fattr->rdev);

		nfsi->read_cache_jiffies = fattr->time_start;
		nfsi->last_updated = jiffies;
#endif
		inode->i_atime = fattr->atime;
		inode->i_mtime = fattr->mtime;
		inode->i_ctime = fattr->ctime;
#ifdef CONFIG_NFS_V4
		if (fattr->valid & NFS_ATTR_FATTR_V4)
			nfsi->change_attr = fattr->change_attr;
#endif
		inode->i_size = nfs_size_to_off_t(fattr->size);
		inode->i_nlink = fattr->nlink;
		inode->i_uid = fattr->uid;
		inode->i_gid = fattr->gid;
		if (fattr->valid & (NFS_ATTR_FATTR_V3 | NFS_ATTR_FATTR_V4)) {
			/*
			 * report the blocks in 512byte units
			 */
			inode->i_blocks = nfs_calc_block_size(fattr->du.nfs3.used);
		} else {
			inode->i_blocks = fattr->du.nfs2.blocks;
		}
#if 0
		nfsi->attrtimeo = NFS_MINATTRTIMEO(inode);
		nfsi->attrtimeo_timestamp = jiffies;
		memset(nfsi->cookieverf, 0, sizeof(nfsi->cookieverf));
		nfsi->access_cache = RB_ROOT;

		nfs_fscache_init_cookie(inode);
#endif
		hsfs_unlock_new_inode(inode);
	} else
		nfs_refresh_inode(inode, fattr);
	DEBUG_OUT("I(%p:%lu)", inode, inode->private);
	
out:
	return inode;

out_no_inode:
//dprintk("nfs_fhget: iget failed with error %ld\n", PTR_ERR(inode));
	goto out;
}

extern int hsi_nfs3_setattr(struct hsfs_inode *, struct nfs_fattr *, struct hsfs_iattr *attr);
#define nfs_inc_stats(x, y) do {} while (0)

#define NFS_VALID_ATTRS (HSFS_ATTR_MODE|HSFS_ATTR_UID|HSFS_ATTR_GID| \
			 HSFS_ATTR_SIZE|HSFS_ATTR_ATIME|HSFS_ATTR_ATIME_SET| \
			 HSFS_ATTR_MTIME|HSFS_ATTR_MTIME_SET)

int
hsi_nfs_setattr(struct hsfs_inode *inode, struct hsfs_iattr *attr)
{
	struct nfs_fattr fattr;
	int error;

	nfs_inc_stats(inode, NFSIOS_VFSSETATTR);

	if (attr->valid & HSFS_ATTR_SIZE) {
		if (!S_ISREG(inode->i_mode) || attr->size == i_size_read(inode))
			attr->valid &= ~HSFS_ATTR_SIZE;
	}

	/* Optimization: if the end result is no change, don't RPC */
	attr->valid &= NFS_VALID_ATTRS;
	if (attr->valid == 0)
		return 0;
#if 0
	lock_kernel();
	/* Write all dirty data */
	filemap_write_and_wait(inode->i_mapping);
	nfs_wb_all(inode);
	/*
	 * Return any delegations if we're going to change ACLs
	 */
	if ((attr->ia_valid & (ATTR_MODE|ATTR_UID|ATTR_GID)) != 0)
		nfs_inode_return_delegation(inode);
	error = NFS_PROTO(inode)->setattr(dentry, &fattr, attr);
#endif
	/* XXX need to change this with NFSv2/v4 supports */
	error = inode->sb->sop->setattr(inode, &fattr, attr);
	if (error == 0)
		nfs_refresh_inode(inode, &fattr);
#if 0
	unlock_kernel();
#endif
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
void nfs_setattr_update_inode(struct hsfs_inode *inode, struct hsfs_iattr *attr)
{
	if ((attr->valid & (HSFS_ATTR_MODE|HSFS_ATTR_UID|HSFS_ATTR_GID)) != 0) {
		if ((attr->valid & HSFS_ATTR_MODE) != 0) {
			int mode = attr->mode & S_IALLUGO;
			mode |= inode->i_mode & ~S_IALLUGO;
			inode->i_mode = mode;
		}
		if ((attr->valid & HSFS_ATTR_UID) != 0)
			inode->i_uid = attr->uid;
		if ((attr->valid & HSFS_ATTR_GID) != 0)
			inode->i_gid = attr->gid;
#if 0
		spin_lock(&inode->i_lock);
		NFS_I(inode)->cache_validity |= NFS_INO_INVALID_ACCESS|NFS_INO_INVALID_ACL;
		spin_unlock(&inode->i_lock);
#endif
	}
	if ((attr->valid & HSFS_ATTR_SIZE) != 0) {
		nfs_inc_stats(inode, NFSIOS_SETATTRTRUNC);
		inode->i_size = attr->size;
#if 0
		nfs_fscache_set_size(inode);
		vmtruncate(inode, attr->ia_size);
#endif
	}
}
#if 0
static int nfs_wait_schedule(void *word)
{
	if (signal_pending(current))
		return -ERESTARTSYS;
	schedule();
	return 0;
}

/*
 * Wait for the inode to get unlocked.
 */
static int nfs_wait_on_inode(struct inode *inode)
{
	struct rpc_clnt	*clnt = NFS_CLIENT(inode);
	struct nfs_inode *nfsi = NFS_I(inode);
	sigset_t oldmask;
	int error;

	rpc_clnt_sigmask(clnt, &oldmask);
	error = wait_on_bit_lock(&nfsi->flags, NFS_INO_REVALIDATING,
					nfs_wait_schedule, TASK_INTERRUPTIBLE);
	rpc_clnt_sigunmask(clnt, &oldmask);

	return error;
}

static void nfs_wake_up_inode(struct inode *inode)
{
	struct nfs_inode *nfsi = NFS_I(inode);

	clear_bit(NFS_INO_REVALIDATING, &nfsi->flags);
	smp_mb__after_clear_bit();
	wake_up_bit(&nfsi->flags, NFS_INO_REVALIDATING);
}

int nfs_getattr(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat)
{
	struct inode *inode = dentry->d_inode;
	int need_atime = NFS_I(inode)->cache_validity & NFS_INO_INVALID_ATIME;
	int err;

	/* Flush out writes to the server in order to update c/mtime */
	nfs_sync_inode_wait(inode, 0, 0, FLUSH_NOCOMMIT);

	/*
	 * We may force a getattr if the user cares about atime.
	 *
	 * Note that we only have to check the vfsmount flags here:
	 *  - NFS always sets S_NOATIME by so checking it would give a
	 *    bogus result
	 *  - NFS never sets MS_NOATIME or MS_NODIRATIME so there is
	 *    no point in checking those.
	 */
 	if ((mnt->mnt_flags & MNT_NOATIME) ||
 	    ((mnt->mnt_flags & MNT_NODIRATIME) && S_ISDIR(inode->i_mode)))
		need_atime = 0;

	if (need_atime)
		err = __nfs_revalidate_inode(NFS_SERVER(inode), inode);
	else
		err = nfs_revalidate_inode(NFS_SERVER(inode), inode);
	if (!err) {
		generic_fillattr(inode, stat);
		stat->ino = nfs_compat_user_ino64(NFS_FILEID(inode));
	}
	return err;
}

static struct nfs_open_context *alloc_nfs_open_context(struct vfsmount *mnt, struct dentry *dentry, struct rpc_cred *cred)
{
	struct nfs_open_context *ctx;

	ctx = (struct nfs_open_context *)kmalloc(sizeof(*ctx), GFP_KERNEL);
	if (ctx != NULL) {
		atomic_set(&ctx->count, 1);
		ctx->path.dentry = dget(dentry);
		ctx->path.mnt = mntget(mnt);
		ctx->cred = get_rpccred(cred);
		ctx->state = NULL;
		ctx->lockowner = current->files;
		ctx->error = 0;
		ctx->dir_cookie = 0;
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
		if (!list_empty(&ctx->list)) {
			struct inode *inode = ctx->path.dentry->d_inode;
			spin_lock(&inode->i_lock);
			list_del(&ctx->list);
			spin_unlock(&inode->i_lock);
		}
		if (ctx->state != NULL)
			nfs4_close_state(&ctx->path, ctx->state, ctx->mode);
		if (ctx->cred != NULL)
			put_rpccred(ctx->cred);
		dput(ctx->path.dentry);
		mntput(ctx->path.mnt);
		kfree(ctx);
	}
}

/*
 * Ensure that mmap has a recent RPC credential for use when writing out
 * shared pages
 */
static void nfs_file_set_open_context(struct file *filp, struct nfs_open_context *ctx)
{
	struct inode *inode = filp->f_dentry->d_inode;
	struct nfs_inode *nfsi = NFS_I(inode);

	filp->private_data = get_nfs_open_context(ctx);
	spin_lock(&inode->i_lock);
	list_add(&ctx->list, &nfsi->open_files);
	spin_unlock(&inode->i_lock);
}

/*
 * Given an inode, search for an open context with the desired characteristics
 */
struct nfs_open_context *nfs_find_open_context(struct inode *inode, struct rpc_cred *cred, int mode)
{
	struct nfs_inode *nfsi = NFS_I(inode);
	struct nfs_open_context *pos, *ctx = NULL;

	spin_lock(&inode->i_lock);
	list_for_each_entry(pos, &nfsi->open_files, list) {
		if (cred != NULL && pos->cred != cred)
			continue;
		if ((pos->mode & mode) == mode) {
			ctx = get_nfs_open_context(pos);
			break;
		}
	}
	spin_unlock(&inode->i_lock);
	return ctx;
}

static void nfs_file_clear_open_context(struct file *filp)
{
	struct inode *inode = filp->f_dentry->d_inode;
	struct nfs_open_context *ctx = (struct nfs_open_context *)filp->private_data;

	if (ctx) {
		filp->private_data = NULL;
		spin_lock(&inode->i_lock);
		list_move_tail(&ctx->list, &NFS_I(inode)->open_files);
		spin_unlock(&inode->i_lock);
		put_nfs_open_context(ctx);
		nfs_fscache_reset_cookie(inode);
	}
}

/*
 * These allocate and release file read/write context information.
 */
int nfs_open(struct inode *inode, struct file *filp)
{
	struct nfs_open_context *ctx;
	struct rpc_cred *cred;

	cred = rpcauth_lookupcred(NFS_CLIENT(inode)->cl_auth, 0);
	if (IS_ERR(cred))
		return PTR_ERR(cred);
	ctx = alloc_nfs_open_context(filp->f_vfsmnt, filp->f_dentry, cred);
	put_rpccred(cred);
	if (ctx == NULL)
		return -ENOMEM;
	ctx->mode = filp->f_mode;
	nfs_file_set_open_context(filp, ctx);
	put_nfs_open_context(ctx);

	nfs_fscache_set_cookie(inode, filp);

	return 0;
}

int nfs_release(struct inode *inode, struct file *filp)
{
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

	dfprintk(PAGECACHE, "NFS: revalidating (%s/%Ld)\n",
		inode->i_sb->s_id, (long long)NFS_FILEID(inode));

	nfs_inc_stats(inode, NFSIOS_INODEREVALIDATE);
	lock_kernel();
	if (!inode || is_bad_inode(inode))
 		goto out_nowait;
	if (NFS_STALE(inode))
 		goto out_nowait;

	status = nfs_wait_on_inode(inode);
	if (status < 0)
		goto out;

	status = -ESTALE;
	if (NFS_STALE(inode))
		goto out;

	status = NFS_PROTO(inode)->getattr(server, NFS_FH(inode), &fattr);
	if (status != 0) {
		dfprintk(PAGECACHE, "nfs_revalidate_inode: (%s/%Ld) getattr failed, error=%d\n",
			 inode->i_sb->s_id,
			 (long long)NFS_FILEID(inode), status);
		if (status == -ESTALE) {
			nfs_zap_caches(inode);
			if (!S_ISDIR(inode->i_mode))
				set_bit(NFS_INO_STALE, &NFS_FLAGS(inode));
		}
		goto out;
	}

	spin_lock(&inode->i_lock);
	status = nfs_update_inode(inode, &fattr);
	if (status) {
		spin_unlock(&inode->i_lock);
		dfprintk(PAGECACHE, "nfs_revalidate_inode: (%s/%Ld) refresh failed, error=%d\n",
			 inode->i_sb->s_id,
			 (long long)NFS_FILEID(inode), status);
		goto out;
	}
	spin_unlock(&inode->i_lock);

	if (nfsi->cache_validity & NFS_INO_INVALID_ACL)
		nfs_zap_acl_cache(inode);

	dfprintk(PAGECACHE, "NFS: (%s/%Ld) revalidation complete\n",
		inode->i_sb->s_id,
		(long long)NFS_FILEID(inode));

 out:
	nfs_wake_up_inode(inode);

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
	if (!(NFS_I(inode)->cache_validity & NFS_INO_INVALID_ATTR)
			&& !nfs_attribute_timeout(inode))
		return NFS_STALE(inode) ? -ESTALE : 0;
	return __nfs_revalidate_inode(server, inode);
}

/**
 * nfs_revalidate_mapping - Revalidate the pagecache
 * @inode - pointer to host inode
 * @mapping - pointer to mapping
 */
int nfs_revalidate_mapping(struct inode *inode, struct address_space *mapping)
{
	struct nfs_inode *nfsi = NFS_I(inode);
	int ret = 0;

	if (NFS_STALE(inode))
		ret = -ESTALE;
	if ((nfsi->cache_validity & NFS_INO_REVAL_PAGECACHE)
			|| nfs_attribute_timeout(inode))
		ret = __nfs_revalidate_inode(NFS_SERVER(inode), inode);

	if (nfsi->cache_validity & NFS_INO_INVALID_DATA) {
		nfs_inc_stats(inode, NFSIOS_DATAINVALIDATE);
		if (S_ISREG(inode->i_mode))
			nfs_sync_mapping(mapping);
		invalidate_inode_pages2(mapping);

		spin_lock(&inode->i_lock);
		nfsi->cache_validity &= ~NFS_INO_INVALID_DATA;
		if (S_ISDIR(inode->i_mode))
			memset(nfsi->cookieverf, 0, sizeof(nfsi->cookieverf));
		spin_unlock(&inode->i_lock);

		nfs_fscache_renew_cookie(inode);

		dfprintk(PAGECACHE, "NFS: (%s/%Ld) data cache invalidated\n",
				inode->i_sb->s_id,
				(long long)NFS_FILEID(inode));
	}
	return ret;
}

static void nfs_wcc_update_inode(struct inode *inode, struct nfs_fattr *fattr)
{
	struct nfs_inode *nfsi = NFS_I(inode);

	if ((fattr->valid & NFS_ATTR_WCC_V4) != 0 &&
			nfsi->change_attr == fattr->pre_change_attr) {
		nfsi->change_attr = fattr->change_attr;
		if (S_ISDIR(inode->i_mode))
			nfsi->cache_validity |= NFS_INO_INVALID_DATA;
	}
	/* If we have atomic WCC data, we may update some attributes */
	if ((fattr->valid & NFS_ATTR_WCC) != 0) {
		if (timespec_equal(&inode->i_ctime, &fattr->pre_ctime))
			memcpy(&inode->i_ctime, &fattr->ctime, sizeof(inode->i_ctime));
		if (timespec_equal(&inode->i_mtime, &fattr->pre_mtime)) {
			memcpy(&inode->i_mtime, &fattr->mtime, sizeof(inode->i_mtime));
			if (S_ISDIR(inode->i_mode))
				nfsi->cache_validity |= NFS_INO_INVALID_DATA;
		}
		if (inode->i_size == fattr->pre_size && nfsi->npages == 0)
			inode->i_size = fattr->size;
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
static int nfs_check_inode_attributes(struct inode *inode, struct nfs_fattr *fattr)
{
	struct nfs_inode *nfsi = NFS_I(inode);
	loff_t cur_size, new_isize;


	/* Has the inode gone and changed behind our back? */
	if (nfsi->fileid != fattr->fileid
			|| (inode->i_mode & S_IFMT) != (fattr->mode & S_IFMT)) {
		return -EIO;
	}

	/* Do atomic weak cache consistency updates */
	nfs_wcc_update_inode(inode, fattr);

	if ((fattr->valid & NFS_ATTR_FATTR_V4) != 0 &&
			nfsi->change_attr != fattr->change_attr)
		nfsi->cache_validity |= NFS_INO_INVALID_ATTR|NFS_INO_REVAL_PAGECACHE;

	/* Verify a few of the more important attributes */
	if (!timespec_equal(&inode->i_mtime, &fattr->mtime))
		nfsi->cache_validity |= NFS_INO_INVALID_ATTR|NFS_INO_REVAL_PAGECACHE;

	cur_size = i_size_read(inode);
 	new_isize = nfs_size_to_loff_t(fattr->size);
	if (cur_size != new_isize && nfsi->npages == 0)
		nfsi->cache_validity |= NFS_INO_INVALID_ATTR|NFS_INO_REVAL_PAGECACHE;

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
	return 0;
}
#endif

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
int nfs_refresh_inode(struct hsfs_inode *inode, struct nfs_fattr *fattr)
{
	struct nfs_inode *nfsi _U_ = NFS_I(inode);
	int status;

	if ((fattr->valid & NFS_ATTR_FATTR) == 0)
		return 0;
#if 0
	spin_lock(&inode->i_lock);
	if (time_after(fattr->time_start, nfsi->last_updated))
#endif
		status = nfs_update_inode(inode, fattr);
#if 0
	else
		status = nfs_check_inode_attributes(inode, fattr);

	spin_unlock(&inode->i_lock);
#endif
	return status;
}
#if 0
/**
 * nfs_post_op_update_inode - try to update the inode attribute cache
 * @inode - pointer to inode
 * @fattr - updated attributes
 *
 * After an operation that has changed the inode metadata, mark the
 * attribute cache as being invalid, then try to update it.
 */
int nfs_post_op_update_inode(struct inode *inode, struct nfs_fattr *fattr)
{
	struct nfs_inode *nfsi = NFS_I(inode);

	if (fattr->valid & NFS_ATTR_FATTR) {
		if (S_ISDIR(inode->i_mode)) {
			spin_lock(&inode->i_lock);
			nfsi->cache_validity |= NFS_INO_INVALID_DATA;
			spin_unlock(&inode->i_lock);
		}
		return nfs_update_inode(inode, fattr);
	}

	spin_lock(&inode->i_lock);
	nfsi->cache_validity |= NFS_INO_INVALID_ACCESS|NFS_INO_INVALID_ATTR|NFS_INO_REVAL_PAGECACHE;
	if (S_ISDIR(inode->i_mode))
		nfsi->cache_validity |= NFS_INO_INVALID_DATA;
	spin_unlock(&inode->i_lock);
	return 0;
}

/**
 * nfs_post_op_update_inode_force_wcc - try to update the inode attribute cache
 * @inode - pointer to inode
 * @fattr - updated attributes
 *
 * After an operation that has changed the inode metadata, mark the
 * attribute cache as being invalid, then try to update it. Fake up
 * weak cache consistency data, if none exist.
 *
 * This function is mainly designed to be used by the ->write_done() functions.
 */
int nfs_post_op_update_inode_force_wcc(struct inode *inode, struct nfs_fattr *fattr)
{
	if ((fattr->valid & NFS_ATTR_FATTR_V4) != 0 &&
			(fattr->valid & NFS_ATTR_WCC_V4) == 0) {
		fattr->pre_change_attr = NFS_I(inode)->change_attr;
		fattr->valid |= NFS_ATTR_WCC_V4;
	}
	if ((fattr->valid & NFS_ATTR_FATTR) != 0 &&
			(fattr->valid & NFS_ATTR_WCC) == 0) {
		memcpy(&fattr->pre_ctime, &inode->i_ctime, sizeof(fattr->pre_ctime));
		memcpy(&fattr->pre_mtime, &inode->i_mtime, sizeof(fattr->pre_mtime));
		fattr->pre_size = inode->i_size;
		fattr->valid |= NFS_ATTR_WCC;
	}
	return nfs_post_op_update_inode(inode, fattr);
}
#endif
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
static int nfs_update_inode(struct hsfs_inode *inode, struct nfs_fattr *fattr)
{
/* 	struct nfs_server *server; */
	struct nfs_inode *nfsi = NFS_I(inode);
	off_t cur_isize _U_ , new_isize;
	unsigned int invalid _U_ = 0;

/* 	DEBUG_IN("NFS: %s(%s/%ld ct=%d info=0x%x)\n", */
/* 		 __FUNCTION__, inode->i_sb->s_id, inode->i_ino, */
/* 		 atomic_read(&inode->i_count), fattr->valid); */

	if (nfsi->fileid != fattr->fileid)
		goto out_fileid;

	/*
	 * Make sure the inode's type hasn't changed.
	 */
	if ((inode->i_mode & S_IFMT) != (fattr->mode & S_IFMT))
		goto out_changed;
#if 0
	server = NFS_SERVER(inode);
	/* Update the fsid? */
	if (S_ISDIR(inode->i_mode) &&
			!nfs_fsid_equal(&server->fsid, &fattr->fsid) &&
			!test_bit(NFS_INO_MOUNTPOINT, &nfsi->flags))
		server->fsid = fattr->fsid;

	/*
	 * Update the read time so we don't revalidate too often.
	 */
	nfsi->read_cache_jiffies = fattr->time_start;
	nfsi->last_updated = jiffies;

	nfsi->cache_validity &= ~(NFS_INO_INVALID_ATTR | NFS_INO_INVALID_ATIME
			| NFS_INO_REVAL_PAGECACHE);

	/* Do atomic weak cache consistency updates */
	nfs_wcc_update_inode(inode, fattr);

	/* More cache consistency checks */
	if (!(fattr->valid & NFS_ATTR_FATTR_V4) != 0) {
		/* NFSv2/v3: Check if the mtime agrees */
		if (!timespec_equal(&inode->i_mtime, &fattr->mtime)) {
			dprintk("NFS: mtime change on server for file %s/%ld\n",
					inode->i_sb->s_id, inode->i_ino);
			invalid |= NFS_INO_INVALID_ATTR|NFS_INO_INVALID_DATA;
			nfsi->cache_change_attribute = jiffies;
		}
		/* If ctime has changed we should definitely clear access+acl caches */
		if (!timespec_equal(&inode->i_ctime, &fattr->ctime))
			invalid |= NFS_INO_INVALID_ACCESS|NFS_INO_INVALID_ACL;
	} else if (nfsi->change_attr != fattr->change_attr) {
		dprintk("NFS: change_attr change on server for file %s/%ld\n",
				inode->i_sb->s_id, inode->i_ino);
		invalid |= NFS_INO_INVALID_ATTR|NFS_INO_INVALID_DATA|NFS_INO_INVALID_ACCESS|NFS_INO_INVALID_ACL;
		nfsi->cache_change_attribute = jiffies;
	}
#endif
	/* Check if our cached file size is stale */
 	new_isize = nfs_size_to_off_t(fattr->size);
#if 0
	cur_isize = i_size_read(inode);
	if (new_isize != cur_isize) {
		/* Do we perhaps have any outstanding writes, or has
		 * the file grown beyond our last write? */
		if (nfsi->npages == 0 || new_isize > cur_isize) {
#endif
			inode->i_size = new_isize;
#if 0
			invalid |= NFS_INO_INVALID_ATTR|NFS_INO_INVALID_DATA;
			nfs_fscache_set_size(inode);
		}
		dprintk("NFS: isize change on server for file %s/%ld\n",
				inode->i_sb->s_id, inode->i_ino);
	}
#endif
	memcpy(&inode->i_mtime, &fattr->mtime, sizeof(inode->i_mtime));
	memcpy(&inode->i_ctime, &fattr->ctime, sizeof(inode->i_ctime));
	memcpy(&inode->i_atime, &fattr->atime, sizeof(inode->i_atime));
#if 0
	nfsi->change_attr = fattr->change_attr;

	if ((inode->i_mode & S_IALLUGO) != (fattr->mode & S_IALLUGO) ||
	    inode->i_uid != fattr->uid ||
	    inode->i_gid != fattr->gid)
		invalid |= NFS_INO_INVALID_ATTR|NFS_INO_INVALID_ACCESS|NFS_INO_INVALID_ACL;
#endif
	inode->i_mode = fattr->mode;
	inode->i_nlink = fattr->nlink;
	inode->i_uid = fattr->uid;
	inode->i_gid = fattr->gid;

	if (fattr->valid & (NFS_ATTR_FATTR_V3 | NFS_ATTR_FATTR_V4)) {
		/*
		 * report the blocks in 512byte units
		 */
		inode->i_blocks = nfs_calc_block_size(fattr->du.nfs3.used);
 	} else {
 		inode->i_blocks = fattr->du.nfs2.blocks;
 	}
#if 0
	/* Update attrtimeo value if we're out of the unstable period */
	if (invalid & NFS_INO_INVALID_ATTR) {
		nfs_inc_stats(inode, NFSIOS_ATTRINVALIDATE);
		nfsi->attrtimeo = NFS_MINATTRTIMEO(inode);
		nfsi->attrtimeo_timestamp = jiffies;
	} else if (time_after(jiffies, nfsi->attrtimeo_timestamp+nfsi->attrtimeo)) {
		if ((nfsi->attrtimeo <<= 1) > NFS_MAXATTRTIMEO(inode))
			nfsi->attrtimeo = NFS_MAXATTRTIMEO(inode);
		nfsi->attrtimeo_timestamp = jiffies;
	}
	invalid &= ~NFS_INO_INVALID_ATTR;
	/* Don't invalidate the data if we were to blame */
	if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode)
				|| S_ISLNK(inode->i_mode)))
		invalid &= ~NFS_INO_INVALID_DATA;
	if (!nfs_have_delegation(inode, FMODE_READ))
		nfsi->cache_validity |= invalid;
#endif
	return 0;
 out_changed:
	/*
	 * Big trouble! The inode has become a different object.
	 */
#ifdef NFS_PARANOIA
	printk(KERN_DEBUG "%s: inode %ld mode changed, %07o to %07o\n",
			__FUNCTION__, inode->i_ino, inode->i_mode, fattr->mode);
#endif
 out_err:
	/*
	 * No need to worry about unhashing the dentry, as the
	 * lookup validation will know that the inode is bad.
	 * (But we fall through to invalidate the caches.)
	 */
/* 	nfs_invalidate_inode(inode); */
	return -ESTALE;

 out_fileid:
/* 	printk(KERN_ERR "NFS: server %s error: fileid changed\n" */
/* 		"fsid %s: expected fileid 0x%Lx, got 0x%Lx\n", */
/* 		NFS_SERVER(inode)->nfs_client->cl_hostname, inode->i_sb->s_id, */
/* 		(long long)nfsi->fileid, (long long)fattr->fileid); */
	goto out_err;
}


#ifdef CONFIG_NFS_V4

/*
 * Clean out any remaining NFSv4 state that might be left over due
 * to open() calls that passed nfs_atomic_lookup, but failed to call
 * nfs_open().
 */
void nfs4_clear_inode(struct inode *inode)
{
	/* If we are holding a delegation, return it! */
	nfs_inode_return_delegation_noreclaim(inode);
	/* First call standard NFS clear_inode() code */
	nfs_clear_inode(inode);
}
#endif

#if 0
static inline void nfs4_init_once(struct nfs_inode *nfsi)
{
#ifdef CONFIG_NFS_V4
	INIT_LIST_HEAD(&nfsi->open_states);
	nfsi->delegation = NULL;
	nfsi->delegation_state = 0;
	init_rwsem(&nfsi->rwsem);
#endif
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
		INIT_LIST_HEAD(&nfsi->access_cache_entry_lru);
		INIT_LIST_HEAD(&nfsi->access_cache_inode_lru);
		INIT_RADIX_TREE(&nfsi->nfs_page_tree, GFP_ATOMIC);
		nfsi->ndirty = 0;
		nfsi->ncommit = 0;
		nfsi->npages = 0;
		atomic_set(&nfsi->silly_count, 1);
		INIT_HLIST_HEAD(&nfsi->silly_list);
		init_waitqueue_head(&nfsi->waitqueue);
		nfs4_init_once(nfsi);
	}
}
 
static int __init nfs_init_inodecache(void)
{
	nfs_inode_cachep = kmem_cache_create("nfs_inode_cache",
					     sizeof(struct nfs_inode),
					     0, (SLAB_RECLAIM_ACCOUNT|
						SLAB_MEM_SPREAD),
					     init_once, NULL);
	if (nfs_inode_cachep == NULL)
		return -ENOMEM;

	return 0;
}

static void nfs_destroy_inodecache(void)
{
	if (kmem_cache_destroy(nfs_inode_cachep))
		printk(KERN_INFO "nfs_inode_cache: not all structures were freed\n");
}

/*
 * Initialize NFS
 */
static int __init init_nfs_fs(void)
{
	int err;

	err = nfs_fscache_register();
	if (err < 0)
		goto out6;

	err = nfs_fs_proc_init();
	if (err)
		goto out5;

	err = nfs_init_nfspagecache();
	if (err)
		goto out4;

	err = nfs_init_inodecache();
	if (err)
		goto out3;

	err = nfs_init_readpagecache();
	if (err)
		goto out2;

	err = nfs_init_writepagecache();
	if (err)
		goto out1;

	err = nfs_init_directcache();
	if (err)
		goto out0;

#ifdef CONFIG_PROC_FS
	rpc_proc_register(&nfs_rpcstat);
#endif
	if ((err = register_nfs_fs()) != 0)
		goto out;
	return 0;
out:
#ifdef CONFIG_PROC_FS
	rpc_proc_unregister("nfs");
#endif
	nfs_destroy_directcache();
out0:
	nfs_destroy_writepagecache();
out1:
	nfs_destroy_readpagecache();
out2:
	nfs_destroy_inodecache();
out3:
	nfs_destroy_nfspagecache();
out4:
	nfs_fs_proc_exit();
out5:
	nfs_fscache_unregister();
out6:
	return err;
}

static void __exit exit_nfs_fs(void)
{
	nfs_destroy_directcache();
	nfs_destroy_writepagecache();
	nfs_destroy_readpagecache();
	nfs_destroy_inodecache();
	nfs_destroy_nfspagecache();
	nfs_fscache_unregister();
#ifdef CONFIG_PROC_FS
	rpc_proc_unregister("nfs");
#endif
	unregister_nfs_fs();
	nfs_fs_proc_exit();
}

/* Not quite true; I just maintain it */
MODULE_AUTHOR("Olaf Kirch <okir@monad.swb.de>");
MODULE_LICENSE("GPL");
module_param(enable_ino64, bool, 0644);

module_init(init_nfs_fs)
module_exit(exit_nfs_fs)
#endif	/* __UNUSED__ */
