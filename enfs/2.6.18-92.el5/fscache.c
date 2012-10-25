/* fscache.c: NFS filesystem cache interface
 *
 * Copyright (C) 2006 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */


#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/nfs_fs.h>
#include <linux/nfs_fs_sb.h>
#include <linux/in6.h>

#include "internal.h"

/*
 * Sysctl variables
 */
atomic_t nfs_fscache_to_pages;
atomic_t nfs_fscache_from_pages;
atomic_t nfs_fscache_uncache_page;
int nfs_fscache_from_error;
int nfs_fscache_to_error;

#define NFSDBG_FACILITY		NFSDBG_FSCACHE

/* the auxiliary data in the cache (used for coherency management) */
struct nfs_fh_auxdata {
	struct timespec	i_mtime;
	struct timespec	i_ctime;
	loff_t		i_size;
};

static struct fscache_netfs_operations nfs_cache_ops = {
};

struct fscache_netfs nfs_cache_netfs = {
	.name			= "nfs",
	.version		= 0,
	.ops			= &nfs_cache_ops,
};

static const uint8_t nfs_cache_ipv6_wrapper_for_ipv4[12] = {
	[0 ... 9]	= 0x00,
	[10 ... 11]	= 0xff
};

struct nfs_server_key {
	uint16_t nfsversion;
	uint16_t port;
	union {
		struct {
			uint8_t		ipv6wrapper[12];
			struct in_addr	addr;
		} ipv4_addr;
		struct in6_addr ipv6_addr;
	};
};

static uint16_t nfs_server_get_key(const void *cookie_netfs_data,
				   void *buffer, uint16_t bufmax)
{
	const struct nfs_client *clp = cookie_netfs_data;
	struct nfs_server_key *key = buffer;
	uint16_t len = 0;

	key->nfsversion = clp->cl_nfsversion;

	switch (clp->cl_addr.sin_family) {
	case AF_INET:
		key->port = clp->cl_addr.sin_port;

		memcpy(&key->ipv4_addr.ipv6wrapper,
		       &nfs_cache_ipv6_wrapper_for_ipv4,
		       sizeof(key->ipv4_addr.ipv6wrapper));
		memcpy(&key->ipv4_addr.addr,
		       &clp->cl_addr.sin_addr,
		       sizeof(key->ipv4_addr.addr));
		len = sizeof(struct nfs_server_key);
		break;

	case AF_INET6:
		key->port = clp->cl_addr.sin_port;

		memcpy(&key->ipv6_addr,
		       &clp->cl_addr.sin_addr,
		       sizeof(key->ipv6_addr));
		len = sizeof(struct nfs_server_key);
		break;

	default:
		len = 0;
		printk(KERN_WARNING "NFS: Unknown network family '%d'\n",
			clp->cl_addr.sin_family);
		break;
	}

	return len;
}

/*
 * the root index for the filesystem is defined by nfsd IP address and ports
 */
struct fscache_cookie_def nfs_cache_server_index_def = {
	.name		= "NFS.servers",
	.type 		= FSCACHE_COOKIE_TYPE_INDEX,
	.get_key	= nfs_server_get_key,
};

static uint16_t nfs_fh_get_key(const void *cookie_netfs_data,
		void *buffer, uint16_t bufmax)
{
	const struct nfs_inode *nfsi = cookie_netfs_data;
	uint16_t nsize;

	/* set the file handle */
	nsize = nfsi->fh.size;
	memcpy(buffer, nfsi->fh.data, nsize);
	return nsize;
}

/*
 * indication of pages that now have cache metadata retained
 * - this function should mark the specified pages as now being cached
 */
static void nfs_fh_mark_pages_cached(void *cookie_netfs_data,
				     struct address_space *mapping,
				     struct pagevec *cached_pvec)
{
	struct nfs_inode *nfsi = cookie_netfs_data;
	unsigned long loop;

	dprintk("NFS: nfs_fh_mark_pages_cached: nfs_inode 0x%p pages %ld\n",
		nfsi, cached_pvec->nr);

	for (loop = 0; loop < cached_pvec->nr; loop++)
		SetPageNfsCached(cached_pvec->pages[loop]);
}

/*
 * get an extra reference on a read context
 * - this function can be absent if the completion function doesn't
 *   require a context
 */
static void nfs_fh_get_context(void *cookie_netfs_data, void *context)
{
	get_nfs_open_context(context);
}

/*
 * release an extra reference on a read context
 * - this function can be absent if the completion function doesn't
 *   require a context
 */
static void nfs_fh_put_context(void *cookie_netfs_data, void *context)
{
	if (context)
		put_nfs_open_context(context);
}

/*
 * indication the cookie is no longer uncached
 * - this function is called when the backing store currently caching a cookie
 *   is removed
 * - the netfs should use this to clean up any markers indicating cached pages
 * - this is mandatory for any object that may have data
 */
static void nfs_fh_now_uncached(void *cookie_netfs_data)
{
	struct nfs_inode *nfsi = cookie_netfs_data;
	struct pagevec pvec;
	pgoff_t first;
	int loop, nr_pages;

	pagevec_init(&pvec, 0);
	first = 0;

	dprintk("NFS: nfs_fh_now_uncached: nfs_inode 0x%p\n", nfsi);

	for (;;) {
		/* grab a bunch of pages to clean */
		nr_pages = pagevec_lookup(&pvec,
					  nfsi->vfs_inode.i_mapping,
					  first,
					  PAGEVEC_SIZE - pagevec_count(&pvec));
		if (!nr_pages)
			break;

		for (loop = 0; loop < nr_pages; loop++)
			ClearPageNfsCached(pvec.pages[loop]);

		first = pvec.pages[nr_pages - 1]->index + 1;

		pvec.nr = nr_pages;
		pagevec_release(&pvec);
		cond_resched();
	}
}

/*
 * get certain file attributes from the netfs data
 * - this function can be absent for an index
 * - not permitted to return an error
 * - the netfs data from the cookie being used as the source is
 *   presented
 */
static void nfs_fh_get_attr(const void *cookie_netfs_data, uint64_t *size)
{
	const struct nfs_inode *nfsi = cookie_netfs_data;

	*size = nfsi->vfs_inode.i_size;
}

/*
 * get the auxilliary data from netfs data
 * - this function can be absent if the index carries no state data
 * - should store the auxilliary data in the buffer
 * - should return the amount of amount stored
 * - not permitted to return an error
 * - the netfs data from the cookie being used as the source is
 *   presented
 */
static uint16_t nfs_fh_get_aux(const void *cookie_netfs_data,
			       void *buffer, uint16_t bufmax)
{
	struct nfs_fh_auxdata auxdata;
	const struct nfs_inode *nfsi = cookie_netfs_data;

	auxdata.i_size = nfsi->vfs_inode.i_size;
	auxdata.i_mtime = nfsi->vfs_inode.i_mtime;
	auxdata.i_ctime = nfsi->vfs_inode.i_ctime;

	if (bufmax > sizeof(auxdata))
		bufmax = sizeof(auxdata);

	memcpy(buffer, &auxdata, bufmax);
	return bufmax;
}

/*
 * consult the netfs about the state of an object
 * - this function can be absent if the index carries no state data
 * - the netfs data from the cookie being used as the target is
 *   presented, as is the auxilliary data
 */
static fscache_checkaux_t nfs_fh_check_aux(void *cookie_netfs_data,
					   const void *data, uint16_t datalen)
{
	struct nfs_fh_auxdata auxdata;
	struct nfs_inode *nfsi = cookie_netfs_data;

	if (datalen > sizeof(auxdata))
		return FSCACHE_CHECKAUX_OBSOLETE;

	auxdata.i_size = nfsi->vfs_inode.i_size;
	auxdata.i_mtime = nfsi->vfs_inode.i_mtime;
	auxdata.i_ctime = nfsi->vfs_inode.i_ctime;

	if (memcmp(data, &auxdata, datalen) != 0)
		return FSCACHE_CHECKAUX_OBSOLETE;

	return FSCACHE_CHECKAUX_OKAY;
}

/*
 * the primary index for each server is simply made up of a series of NFS file
 * handles
 */
struct fscache_cookie_def nfs_cache_fh_index_def = {
	.name			= "NFS.fh",
	.type			= FSCACHE_COOKIE_TYPE_DATAFILE,
	.get_key		= nfs_fh_get_key,
	.get_attr		= nfs_fh_get_attr,
	.get_aux		= nfs_fh_get_aux,
	.check_aux		= nfs_fh_check_aux,
	.get_context		= nfs_fh_get_context,
	.put_context		= nfs_fh_put_context,
	.mark_pages_cached	= nfs_fh_mark_pages_cached,
	.now_uncached		= nfs_fh_now_uncached,
};

static int nfs_file_page_mkwrite(struct vm_area_struct *vma, struct page *page)
{
	wait_on_page_fs_misc(page);
	return 0;
}

struct vm_operations_struct nfs_fs_vm_operations = {
	.nopage		= filemap_nopage,
	.populate	= filemap_populate,
	.page_mkwrite	= nfs_file_page_mkwrite,
};

/*
 * handle completion of a page being stored in the cache
 */
void nfs_readpage_to_fscache_complete(struct page *page, void *data, int error)
{
	dfprintk(FSCACHE,
		"NFS:     readpage_to_fscache_complete (p:%p(i:%lx f:%lx)/%d)\n",
		page, page->index, page->flags, error);

	end_page_fs_misc(page);
}

/*
 * handle completion of a page being read from the cache
 * - called in process (keventd) context
 */
void nfs_readpage_from_fscache_complete(struct page *page,
					void *context,
					int error)
{
	dfprintk(FSCACHE,
		 "NFS: readpage_from_fscache_complete (0x%p/0x%p/%d)\n",
		 page, context, error);

	/* if the read completes with an error, we just unlock the page and let
	 * the VM reissue the readpage */
	if (!error) {
		SetPageUptodate(page);
		unlock_page(page);
	} else {
		error = nfs_readpage_async(context, page->mapping->host, page);
		if (error)
			unlock_page(page);
	}
}

/*
 * handle completion of a page being read from the cache
 * - really need to synchronise the end of writeback, probably using a page
 *   flag, but for the moment we disable caching on writable files
 */
void nfs_writepage_to_fscache_complete(struct page *page,
				       void *data,
				       int error)
{
}
