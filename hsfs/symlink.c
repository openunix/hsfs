/*
 *  linux/fs/nfs/symlink.c
 *
 *  Copyright (C) 1992  Rick Sladkey
 *
 *  Optimization changes Copyright (C) 1994 Florian La Roche
 *
 *  Jun 7 1999, cache symlink lookups in the page cache.  -DaveM
 *
 *  nfs symlink handling code
 */

#define NFS_NEED_XDR_TYPES
#include <linux/time.h>
#include <linux/errno.h>
#include <linux/sunrpc/clnt.h>
/* qinping mod 20060331 begin */
//#include <linux/nfs.h>
//#include <linux/nfs2.h>
//#include <linux/nfs_fs.h>
#include "nfs.h"
#include "nfs2.h"
#include "nfs_fs.h"
/* qinping mod 20060331 end */
#include <linux/pagemap.h>
#include <linux/stat.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/smp_lock.h>
#include <linux/namei.h>

/* Symlink caching in the page cache is even more simplistic
 * and straight-forward than readdir caching.
 *
 * At the beginning of the page we store pointer to struct page in question,
 * simplifying nfs_put_link() (if inode got invalidated we can't find the page
 * to be freed via pagecache lookup).
 * The NUL-terminated string follows immediately thereafter.
 */
struct nfs_symlink {
	struct page *page;
	char body[0];
};
static int nfs_symlink_filler(struct inode *inode, struct page *page)
{
#ifndef __SUSE9_SUP
	const unsigned int pgbase = offsetof(struct nfs_symlink, body);
	const unsigned int pglen = PAGE_SIZE - pgbase;
#else
	const unsigned int pgbase = 0;
	const unsigned int pglen = PAGE_SIZE ;
#endif
	int error;
	lock_kernel();
	error = NFS_PROTO(inode)->readlink(inode, page, pgbase, pglen);
	unlock_kernel();
	if (error < 0)
		goto error;
	SetPageUptodate(page);
	unlock_page(page);
	return 0;

error:
	SetPageError(page);
	unlock_page(page);
	return -EIO;
}
#ifdef __SUSE9_SUP
static char *nfs_getlink(struct inode *inode, struct page **ppage)
{
	struct page *page;
	u32 *p;

	page = ERR_PTR(nfs_revalidate_inode(NFS_SERVER(inode), inode));
	if (page)
		goto read_failed;
	page = read_cache_page(&inode->i_data, 0,
				(filler_t *)nfs_symlink_filler, inode);
	if (IS_ERR(page))
		goto read_failed;
	if (!PageUptodate(page))
		goto getlink_read_error;
	*ppage = page;
	p = kmap(page);
	return (char*)p;
/*
	zdz comment 22060901
	SUSE use p+1,we change it to p.
	return (char*)(p+1);
*/
getlink_read_error:
	page_cache_release(page);
	page = ERR_PTR(-EIO);
read_failed:
	return (char*)page;
}

static int nfs_readlink(struct dentry *dentry, char __user *buffer, int buflen)
{
	struct inode *inode = dentry->d_inode;
	struct page *page = NULL;
	int res = vfs_readlink(dentry,buffer,buflen,nfs_getlink(inode,&page));
	if (page) {
		kunmap(page);
		page_cache_release(page);
	}
	return res;
}

static int nfs_follow_link(struct dentry *dentry, struct nameidata *nd)
{
	struct inode *inode = dentry->d_inode;
	struct page *page = NULL;
	int res = vfs_follow_link(nd, nfs_getlink(inode,&page));
	if (page) {
		kunmap(page);
		page_cache_release(page);
	}
	return res;
}
#else 
static int nfs_follow_link(struct dentry *dentry, struct nameidata *nd)
{
	struct inode *inode = dentry->d_inode;
	struct page *page;
	struct nfs_symlink *p;
	void *err = ERR_PTR(nfs_revalidate_inode(NFS_SERVER(inode), inode));
	if (err)
		goto read_failed;
	page = read_cache_page(&inode->i_data, 0,
				(filler_t *)nfs_symlink_filler, inode);
	if (IS_ERR(page)) {
		err = page;
		goto read_failed;
	}
	if (!PageUptodate(page)) {
		err = ERR_PTR(-EIO);
		goto getlink_read_error;
	}
	p = kmap(page);
	p->page = page;
	nd_set_link(nd, p->body);
	return 0;

getlink_read_error:
	page_cache_release(page);
read_failed:
	nd_set_link(nd, err);
	return 0;
}

static void nfs_put_link(struct dentry *dentry, struct nameidata *nd)
{
	char *s = nd_get_link(nd);
	if (!IS_ERR(s)) {
		struct nfs_symlink *p;
		struct page *page;

		p = container_of(s, struct nfs_symlink, body[0]);
		page = p->page;

		kunmap(page);
		page_cache_release(page);
	}
}
#endif  /*__SUSE9_SUP*/
/*
 * symlinks can't do much...
 */
struct inode_operations nfs_symlink_inode_operations = {
#ifdef __SUSE9_SUP
	.readlink	= nfs_readlink,
#else
	.readlink	= generic_readlink,
#endif   /*__SUSE9_SUP*/
	.follow_link	= nfs_follow_link,
#ifndef __SUSE9_SUP
	.put_link	= nfs_put_link,
#endif  /*__SUSE9_SUP*/
	.getattr	= nfs_getattr,
	.setattr	= nfs_setattr,
};
