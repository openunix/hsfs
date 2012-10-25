/* fscache.h: NFS filesystem cache interface definitions
 *
 * Copyright (C) 2006 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef _NFS_FSCACHE_H
#define _NFS_FSCACHE_H

#include <linux/nfs_fs.h>
#include <linux/nfs_mount.h>
#include <linux/nfs4_mount.h>

#ifdef CONFIG_NFS_FSCACHE
#include <linux/fscache.h>

extern struct fscache_netfs nfs_cache_netfs;
extern struct fscache_cookie_def nfs_cache_server_index_def;
extern struct fscache_cookie_def nfs_cache_fh_index_def;
extern struct vm_operations_struct nfs_fs_vm_operations;

extern void nfs_invalidatepage(struct page *, unsigned long);
extern int nfs_releasepage(struct page *, gfp_t);

extern atomic_t nfs_fscache_to_pages;
extern atomic_t nfs_fscache_from_pages;
extern atomic_t nfs_fscache_uncache_page;
extern int nfs_fscache_from_error;
extern int nfs_fscache_to_error;

/*
 * register NFS for caching
 */
static inline int nfs_fscache_register(void)
{
	return fscache_register_netfs(&nfs_cache_netfs);
}

/*
 * unregister NFS for caching
 */
static inline void nfs_fscache_unregister(void)
{
	fscache_unregister_netfs(&nfs_cache_netfs);
}
/*
 * Initialize the fsc cookie and set the cache-able 
 * bit if need be. 
 */
static inline void nfs_fscache_init_cookie(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;

	NFS_I(inode)->fscache = NULL;
	if (NFS_SB(sb)->flags & NFS_MOUNT_FSCACHE) {
		if (S_ISREG(inode->i_mode))
				set_bit(NFS_INO_FSCACHE, &NFS_FLAGS(inode));
	}
}

/*
 * get the per-client index cookie for an NFS client if the appropriate mount
 * flag was set
 * - we always try and get an index cookie for the client, but get filehandle
 *   cookies on a per-superblock basis, depending on the mount flags
 */
static inline void nfs_fscache_get_client_cookie(struct nfs_client *clp)
{
	/* create a cache index for looking up filehandles */
	clp->fscache = fscache_acquire_cookie(nfs_cache_netfs.primary_index,
					      &nfs_cache_server_index_def,
					      clp);
	dfprintk(FSCACHE,"NFS: get client cookie (0x%p/0x%p)\n",
		 clp, clp->fscache);
}

/*
 * dispose of a per-client cookie
 */
static inline void nfs_fscache_release_client_cookie(struct nfs_client *clp)
{
	dfprintk(FSCACHE,"NFS: releasing client cookie (0x%p/0x%p)\n",
		clp, clp->fscache);

	fscache_relinquish_cookie(clp->fscache, 0);
	clp->fscache = NULL;
}

/*
 * indicate the client caching state as readable text
 */
static inline const char *nfs_server_fscache_state(struct nfs_server *server)
{
	if (server->nfs_client->fscache && (server->flags & NFS_MOUNT_FSCACHE))
		return "yes";
	return "no ";
}

/*
 * get the per-filehandle cookie for an NFS inode
 */
static inline void nfs_fscache_enable_cookie(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;
	struct nfs_inode *nfsi = NFS_I(inode);

	if (nfsi->fscache != NULL)
		return;

	if ((NFS_SB(sb)->flags & NFS_MOUNT_FSCACHE)) {
		nfsi->fscache = fscache_acquire_cookie(
			NFS_SB(sb)->nfs_client->fscache,
			&nfs_cache_fh_index_def,
			nfsi);

		fscache_set_i_size(nfsi->fscache, nfsi->vfs_inode.i_size);

		dfprintk(FSCACHE, "NFS: get FH cookie (0x%p/0x%p/0x%p)\n",
			 sb, nfsi, nfsi->fscache);
	}
}

/*
 * change the filesize associated with a per-filehandle cookie
 */
static inline void nfs_fscache_set_size(struct inode *inode)
{
	fscache_set_i_size(NFS_I(inode)->fscache, inode->i_size);
}

/*
 * replace a per-filehandle cookie due to revalidation detecting a file having
 * changed on the server
 */
static inline void nfs_fscache_renew_cookie(struct inode *inode)
{
	struct nfs_inode *nfsi = NFS_I(inode);
	struct nfs_server *server = NFS_SERVER(inode);
	struct fscache_cookie *old = nfsi->fscache;

	if (nfsi->fscache) {
		/* retire the current fscache cache and get a new one */
		fscache_relinquish_cookie(nfsi->fscache, 1);

		nfsi->fscache = fscache_acquire_cookie(
			server->nfs_client->fscache,
			&nfs_cache_fh_index_def,
			nfsi);
		fscache_set_i_size(nfsi->fscache, nfsi->vfs_inode.i_size);

		dfprintk(FSCACHE,
			 "NFS: revalidation new cookie (0x%p/0x%p/0x%p/0x%p)\n",
			 server, nfsi, old, nfsi->fscache);
	}
}

/*
 * release a per-filehandle cookie
 */
static inline void nfs_fscache_release_cookie(struct inode *inode)
{
	struct nfs_inode *nfsi = NFS_I(inode);

	dfprintk(FSCACHE, "NFS: clear cookie (0x%p/0x%p)\n",
		 nfsi, nfsi->fscache);

	fscache_relinquish_cookie(nfsi->fscache, 0);
	nfsi->fscache = NULL;
}

/*
 * retire a per-filehandle cookie, destroying the data attached to it
 */
static inline void nfs_fscache_zap_cookie(struct inode *inode)
{
	struct nfs_inode *nfsi = NFS_I(inode);

	dfprintk(FSCACHE,"NFS: zapping cookie (0x%p/0x%p)\n",
		nfsi, nfsi->fscache);

	fscache_relinquish_cookie(nfsi->fscache, 1);
	nfsi->fscache = NULL;
}

/*
 * turn off the cache with regard to a filehandle cookie if opened for writing,
 * invalidating all the pages in the page cache relating to the associated
 * inode to clear the per-page caching
 */
static inline void nfs_fscache_disable_cookie(struct inode *inode)
{
	if (NFS_I(inode)->fscache) {
		dfprintk(FSCACHE,
			 "NFS: nfsi 0x%p turning cache off\n", NFS_I(inode));

		/* Need to invalided any mapped pages that were read in before
		 * turning off the cache.
		 */
		if (inode->i_mapping && inode->i_mapping->nrpages)
			invalidate_inode_pages2(inode->i_mapping);

		nfs_fscache_zap_cookie(inode);
	}
}

/*
 * Decide if we should enable or disable the FS cache for
 * this inode. For now, only files that are open for read
 * will be able to use the FS cache.
 */
static inline void nfs_fscache_set_cookie(struct inode *inode, 
	struct file *filp)
{
	struct super_block *sb = inode->i_sb;

	if (!(NFS_SB(sb)->flags & NFS_MOUNT_FSCACHE))
		return;

	if (!NFS_FSCACHE(inode))
		return;

	if ((filp->f_flags & O_ACCMODE) != O_RDONLY) {
		nfs_fscache_disable_cookie(inode);
		clear_bit(NFS_INO_FSCACHE, &NFS_FLAGS(inode));
	} else
		nfs_fscache_enable_cookie(inode);
}

/*
 * On the last close, re-enable the cache if need be.
 */
static inline void nfs_fscache_reset_cookie(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;

	if (NFS_SB(sb)->flags & NFS_MOUNT_FSCACHE) {
		if (list_empty(&NFS_I(inode)->open_files))
			set_bit(NFS_INO_FSCACHE, &NFS_FLAGS(inode));
	}
}

/*
 * install the VM ops for mmap() of an NFS file so that we can hold up writes
 * to pages on shared writable mappings until the store to the cache is
 * complete
 */
static inline void nfs_fscache_install_vm_ops(struct inode *inode,
					      struct vm_area_struct *vma)
{
	if (NFS_I(inode)->fscache)
		vma->vm_ops = &nfs_fs_vm_operations;
}

/*
 * release the caching state associated with a page
 */
static void nfs_fscache_release_page(struct page *page)
{
	if (PageNfsCached(page)) {
		struct nfs_inode *nfsi = NFS_I(page->mapping->host);

		BUG_ON(nfsi->fscache == NULL);

		dfprintk(FSCACHE, "NFS: fscache releasepage (0x%p/0x%p/0x%p)\n",
			 nfsi->fscache, page, nfsi);

		wait_on_page_fs_misc(page);
		fscache_uncache_page(nfsi->fscache, page);
		atomic_inc(&nfs_fscache_uncache_page);
		ClearPageNfsCached(page);
	}
}

/*
 * release the caching state associated with a page if undergoing complete page
 * invalidation
 */
static inline void nfs_fscache_invalidate_page(struct page *page,
					       struct inode *inode,
					       unsigned long offset)
{
	if (PageNfsCached(page)) {
		struct nfs_inode *nfsi = NFS_I(page->mapping->host);

		BUG_ON(!nfsi->fscache);

		dfprintk(FSCACHE,
			 "NFS: fscache invalidatepage (0x%p/0x%p/0x%p)\n",
			 nfsi->fscache, page, nfsi);

		if (offset == 0) {
			BUG_ON(!PageLocked(page));
			if (!PageWriteback(page))
				nfs_fscache_release_page(page);
		}
	}
}

/*
 * store a newly fetched page in fscache
 */
extern void nfs_readpage_to_fscache_complete(struct page *, void *, int);

static inline void nfs_readpage_to_fscache(struct inode *inode,
					   struct page *page,
					   int sync)
{
	int ret;

	if (PageNfsCached(page)) {
		dfprintk(FSCACHE,
			 "NFS: "
			 "readpage_to_fscache(fsc:%p/p:%p(i:%lx f:%lx)/%d)\n",
			 NFS_I(inode)->fscache, page, page->index, page->flags,
			 sync);

		if (TestSetPageFsMisc(page))
			BUG();

		ret = fscache_write_page(NFS_I(inode)->fscache, page,
					 nfs_readpage_to_fscache_complete,
					 NULL, GFP_KERNEL);
		dfprintk(FSCACHE,
			 "NFS:     "
			 "readpage_to_fscache: p:%p(i:%lu f:%lx) ret %d\n",
			 page, page->index, page->flags, ret);

		if (ret != 0) {
			fscache_uncache_page(NFS_I(inode)->fscache, page);
			atomic_inc(&nfs_fscache_uncache_page);
			ClearPageNfsCached(page);
			end_page_fs_misc(page);
			nfs_fscache_to_error = ret;
		} else {
			atomic_inc(&nfs_fscache_to_pages);
		}
	}
}

/*
 * retrieve a page from fscache
 */
extern void nfs_readpage_from_fscache_complete(struct page *, void *, int);

static inline
int nfs_readpage_from_fscache(struct nfs_open_context *ctx,
			      struct inode *inode,
			      struct page *page)
{
	int ret;

	if (!NFS_I(inode)->fscache)
		return 1;

	dfprintk(FSCACHE,
		 "NFS: readpage_from_fscache(fsc:%p/p:%p(i:%lx f:%lx)/0x%p)\n",
		 NFS_I(inode)->fscache, page, page->index, page->flags, inode);

	ret = fscache_read_or_alloc_page(NFS_I(inode)->fscache,
					 page,
					 nfs_readpage_from_fscache_complete,
					 ctx,
					 GFP_KERNEL);

	switch (ret) {
	case 0: /* read BIO submitted (page in fscache) */
		dfprintk(FSCACHE,
			 "NFS:    readpage_from_fscache: BIO submitted\n");
		atomic_inc(&nfs_fscache_from_pages);
		return ret;

	case -ENOBUFS: /* inode not in cache */
	case -ENODATA: /* page not in cache */
		dfprintk(FSCACHE,
			 "NFS:    readpage_from_fscache error %d\n", ret);
		return 1;

	default:
		dfprintk(FSCACHE, "NFS:    readpage_from_fscache %d\n", ret);
		nfs_fscache_from_error = ret;
	}
	return ret;
}

/*
 * retrieve a set of pages from fscache
 */
static inline int nfs_readpages_from_fscache(struct nfs_open_context *ctx,
					     struct inode *inode,
					     struct address_space *mapping,
					     struct list_head *pages,
					     unsigned *nr_pages)
{
	int ret, npages = *nr_pages;

	if (!NFS_I(inode)->fscache)
		return 1;

	dfprintk(FSCACHE,
		 "NFS: nfs_getpages_from_fscache (0x%p/%u/0x%p)\n",
		 NFS_I(inode)->fscache, *nr_pages, inode);

	ret = fscache_read_or_alloc_pages(NFS_I(inode)->fscache,
					  mapping, pages, nr_pages,
					  nfs_readpage_from_fscache_complete,
					  ctx,
					  mapping_gfp_mask(mapping));


	switch (ret) {
	case 0: /* read BIO submitted (page in fscache) */
		BUG_ON(!list_empty(pages));
		BUG_ON(*nr_pages != 0);
		dfprintk(FSCACHE,
			 "NFS: nfs_getpages_from_fscache: BIO submitted\n");

		atomic_add(npages, &nfs_fscache_from_pages);
		return ret;

	case -ENOBUFS: /* inode not in cache */
	case -ENODATA: /* page not in cache */
		dfprintk(FSCACHE,
			 "NFS: nfs_getpages_from_fscache: no page: %d\n", ret);
		return 1;

	default:
		dfprintk(FSCACHE,
			 "NFS: nfs_getpages_from_fscache: ret  %d\n", ret);
		nfs_fscache_from_error = ret;
	}

	return ret;
}

/*
 * store an updated page in fscache
 */
extern void nfs_writepage_to_fscache_complete(struct page *page, void *data, int error);

static inline void nfs_writepage_to_fscache(struct inode *inode,
					    struct page *page)
{
	int error;

	if (PageNfsCached(page) && NFS_I(inode)->fscache) {
		dfprintk(FSCACHE,
			 "NFS: writepage_to_fscache (0x%p/0x%p/0x%p)\n",
			 NFS_I(inode)->fscache, page, inode);

		error = fscache_write_page(NFS_I(inode)->fscache, page,
					   nfs_writepage_to_fscache_complete,
					   NULL, GFP_KERNEL);
		if (error != 0) {
			dfprintk(FSCACHE,
				 "NFS:    fscache_write_page error %d\n",
				 error);
			fscache_uncache_page(NFS_I(inode)->fscache, page);
		}
	}
}

#else /* CONFIG_NFS_FSCACHE */
static inline int nfs_fscache_register(void) { return 0; }
static inline void nfs_fscache_unregister(void) {}
static inline void nfs_fscache_init_cookie(struct inode *inode) {}
static inline void nfs_fscache_get_client_cookie(struct nfs_client *clp) {}
static inline void nfs4_fscache_get_client_cookie(struct nfs_client *clp) {}
static inline void nfs_fscache_release_client_cookie(struct nfs_client *clp) {}
static inline const char *nfs_server_fscache_state(struct nfs_server *server) { return "no "; }
static inline void nfs_fscache_enable_cookie(struct inode *inode) { }
static inline void nfs_fscache_set_size(struct inode *inode) { }
static inline void nfs_fscache_release_cookie(struct inode *inode) {}
static inline void nfs_fscache_zap_cookie(struct inode *inode) {}
static inline void nfs_fscache_renew_cookie(struct inode *inode) {}
static inline void nfs_fscache_disable_cookie(struct inode *inode) {}
static inline void nfs_fscache_set_cookie(struct inode *inode, struct file *filp) {}
static inline void nfs_fscache_reset_cookie(struct inode *inode) {}
static inline void nfs_fscache_install_vm_ops(struct inode *inode, struct vm_area_struct *vma) {}
static inline void nfs_fscache_release_page(struct page *page) {}
static inline void nfs_fscache_invalidate_page(struct page *page,
					       struct inode *inode,
					       unsigned long offset)
{
}
static inline void nfs_readpage_to_fscache(struct inode *inode, struct page *page, int sync) {}
static inline int nfs_readpage_from_fscache(struct nfs_open_context *ctx,
					    struct inode *inode, struct page *page)
{
	return -ENOBUFS;
}
static inline int nfs_readpages_from_fscache(struct nfs_open_context *ctx,
					     struct inode *inode,
					     struct address_space *mapping,
					     struct list_head *pages,
					     unsigned *nr_pages)
{
	return -ENOBUFS;
}

static inline void nfs_writepage_to_fscache(struct inode *inode, struct page *page)
{
	BUG_ON(PageNfsCached(page));
}

#endif /* CONFIG_NFS_FSCACHE */
#endif /* _NFS_FSCACHE_H */
