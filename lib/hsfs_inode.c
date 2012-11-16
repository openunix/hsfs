/*
 * Copyright (C) 2012 Feng Shuo <steve.shuo.feng@gmail.com>
 *
 * This file is part of HSFS and based on Linux fs/inode.c.
 *
 * HSFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include <hsfs.h>


unsigned int HSFS_PAGE_SIZE;
int hsfs_init()
{
	HSFS_PAGE_SIZE = sysconf(_SC_PAGE_SIZE);

	return 0;
}

struct hsfs_inode bad_hsfs_inode = {
	.ino = 0,
};

int hsfs_init_icache(struct hsfs_super *sb)
{
	hash_init(sb->id_table);
	hash_init(sb->fh_table);
	return 0;
}

static void wait_on_inode(struct hsfs_inode *inode __attribute__((unused)))
{
	;
}

static void wake_up_inode(struct hsfs_inode *inode __attribute__((unused)))
{
	;
}

static void generic_delete_inode(struct hsfs_inode *inode __attribute__((unused)))
{
	;
}

static void __iget(struct hsfs_inode *inode __attribute__((unused)))
{
	;
}

/* This is equal to Linux ifind_fast() but without __iget() called. */
static struct hsfs_inode *__id_ifind(struct hsfs_super *sb, uint64_t key)
{
	struct hsfs_inode *inode = NULL;
	struct hlist_node *node = NULL;
	
	/* XXX LOCK */
	hash_for_each_possible(sb->id_table, inode, node, id_hash, key){
		if (inode->ino != key)
			continue;
		/* XXX Check the indoe status? */
		break;
	}
	
	return inode;
}

/* This is just the same as ifind() of Linux kernel */
static struct hsfs_inode *
__fh_ifind(struct hsfs_super *sb, uint64_t key,
	   int (*test)(struct hsfs_inode *, void *),
	   void *data, const int wait)
{
	struct hsfs_inode *inode = NULL;
	struct hlist_node *node = NULL;
	
	/* XXX LOCK */
	hash_for_each_possible(sb->fh_table, inode, node, fh_hash, key){
		if (!test(inode, data))
			continue;
		/* XXX Check the indoe status? */
		break;
	}
	if (node){
 		__iget(inode);
		/* UNLOCK */
		if (wait)
			wait_on_inode(inode);
		return inode;
	}
	/* UNLOCK */
	return NULL;
}

/* Linux: ilookup */
struct hsfs_inode *hsfs_ilookup(struct hsfs_super *sb, uint64_t ino)
{
	struct hsfs_inode *inode;

	inode = __id_ifind(sb, ino);
	if (inode)
		__iget(inode);

	return inode;
}

/* Linux: destroy_inode */
static void destroy_inode(struct hsfs_inode *inode)
{
	/* XXX Need more check: __destroy_inode(inode); */
	if (inode->sb->sop->destroy_inode)
		inode->sb->sop->destroy_inode(inode);
	else
		free(inode);
}

/* Linux: inode_init_always() */
static int inode_init_always(struct hsfs_super *sb, struct hsfs_inode *inode)
{
	inode->sb = sb;
	inode->generation = 0;
	inode->private = NULL;
	inode->ino = 0;
	inode->i_blocks = 0;
	inode->i_nlink = 1;


	return 0;
}

/* Linux: alloc_inode() */
static struct hsfs_inode *alloc_inode(struct hsfs_super *sb)
{
	struct hsfs_inode *inode;

	if(sb->sop->alloc_inode)
		inode = sb->sop->alloc_inode(sb);
	else
		inode = malloc(sizeof(struct hsfs_inode));

	if (!inode)
		return NULL;

	if (unlikely(inode_init_always(sb, inode))) {
		if (inode->sb->sop->destroy_inode)
			inode->sb->sop->destroy_inode(inode);
		else
			free(inode);
		return NULL;
	}

	return inode;
}

/* Linux: __inode_add_to_lists */
static inline void
__inode_add_to_lists(struct hsfs_super *sb, uint64_t key, struct hsfs_inode *inode)
{
	
	do {
		sb->curr_id++;
		if (!sb->curr_id)
			sb->curr_id = 2;
		assert(sb->curr_id != 1);
	} while (__id_ifind(sb, sb->curr_id) != NULL);
	inode->ino = sb->curr_id;

	hash_add(sb->id_table, &inode->id_hash, inode->ino);
	hash_add(sb->fh_table, &inode->fh_hash, key);
}

/* Linux: get_new_inode() */
static struct hsfs_inode *
get_new_inode(struct hsfs_super *sb, uint64_t key,
	      int (*test)(struct hsfs_inode *, void *) __attribute__((unused)),
	      int (*set)(struct hsfs_inode *, void *),
	      void *data)
{
	struct hsfs_inode *inode;
	
	inode = alloc_inode(sb);
	if (inode) {
		struct hsfs_inode *old = NULL;
	
		/* XXX Need more checks if there are locks.... */
		if (!old){
			if (set(inode, data))
				goto set_failed;

			__inode_add_to_lists(sb, key, inode);
			inode->i_state = I_NEW;

			return inode;
		}
		/* XXX Need more checks if there are locks.... */		
	}
	return inode;

set_failed:
	destroy_inode(inode);
	return NULL;
}

/* Linux: iget5_locked() */
struct hsfs_inode *
hsfs_iget5_locked(struct hsfs_super *sb, uint64_t hashval,
		  int (*test)(struct hsfs_inode *, void *),
		  int (*set)(struct hsfs_inode *, void *), void *data)
{
	struct hsfs_inode *inode;
	
	inode = __fh_ifind(sb, hashval, test, data, 1);
	if (inode)
		return inode;
	
 	return get_new_inode(sb, hashval, test, set, data);
}
static void generic_forget_inode(struct hsfs_inode *inode)
{
#if 0
	if (!generic_detach_inode(inode))
		return;
	if (inode->i_data.nrpages)
		truncate_inode_pages(&inode->i_data, 0);
	clear_inode(inode);
#endif
	wake_up_inode(inode);
	destroy_inode(inode);
}

/*
 * Normal UNIX filesystem behaviour: delete the
 * inode when the usage count drops to zero, and
 * i_nlink is zero.
 */
void generic_drop_inode(struct hsfs_inode *inode)
{
	if (!inode->i_nlink)
		generic_delete_inode(inode);
	else
		generic_forget_inode(inode);
}

static inline void iput_final(struct hsfs_inode *inode)
{
	const struct hsfs_super_ops *op = inode->sb->sop;
	void (*drop)(struct hsfs_inode *) = generic_drop_inode;

	if (op && op->drop_inode)
		drop = op->drop_inode;
	drop(inode);
}

void hsfs_iput(struct hsfs_inode *inode)
{
	if (inode) {
		/* BUG_ON(inode->i_state == I_CLEAR); */
		inode->i_count--;
		/* if (atomic_dec_and_lock(&inode->i_count, &inode_lock)) */
		if (!inode->i_count)
			iput_final(inode);
	}
}

