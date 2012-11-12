/*
 * Copyright (C) 2012 Feng Shuo <steve.shuo.feng@gmail.com>
 *
 * This file is part of HSFS.
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
struct hsfs_inode bad_hsfs_inode = {
	.ino = 0,
};

int hsfs_init_icache(struct hsfs_super *sb)
{
	hash_init(sb->id_table);
	hash_init(sb->fh_table);
	return 1;
}

static void wait_on_inode(struct hsfs_inode *inode __attribute__((unused)))
{
	;
}

static void __iget(struct hsfs_inode *inode __attribute__((unused)))
{
	;
}

/* This is just the same as ifind() of Linux kernel */
struct hsfs_inode *__fh_ifind(struct hsfs_super *sb, unsigned long key,
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

struct hsfs_inode *hsfs_ilookup(struct hsfs_super *sb, uint64_t ino)
{
	return __hsfs_ilookup(sb, ino);
}

int inode_init_always(struct hsfs_super *sb, struct hsfs_inode *inode)
{
	return 0;
}

static struct hsfs_inode *alloc_inode(struct hsfs_super *sb)
{
	struct hsfs_inode *inode;

	if(sb->s_op->alloc_inode)
		inode = sb->s_op->alloc_inode(sb);
	else
		inode = malloc(sizeof(struct hsfs_inode));

	if (!inode)
		return NULL;

	if (inode_init_always(sb, inode)) {
		if (inode->i_sb->s_op->destropy_inode)
			inode->i_sb->s_op->destropy_inode(inode);
		else
			free(inode);
		return NULL;
	}

	return inode;
}

/* This is actually empty. Need more codes if there are locks.... */
static struct hsfs_inode *get_new_inode(struct hsfs_super *sb )
{
	struct hsfs_inode *inode;

	inode = alloc_inode(sb);
	
	return inode;
}

/* This is just the same as iget5_locked of Linux kernel */
struct hsfs_inode *hsfs_iget5_locked(struct hsfs_super *sb, unsigned long hashval,
				     int (*test)(struct hsfs_inode *, void *),
				     int (*set)(struct hsfs_inode *, void *),
				     void *data)
{
	struct hsfs_inode *inode;

	inode = __fh_ifind(sb, hashval, test, data, 1);
	if (inode)
		return inode;

	return get_new_inode(sb);
/* 	return get_new_inode(sb, head, test, set, data); */
}
