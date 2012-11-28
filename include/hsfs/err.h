/*
 * Copyright (C) 2012 Feng Shuo <steve.shuo.feng@gmail.com>
 *
 * This file is part of HSFS and based on linux include/linux/err.h
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

#ifndef _HSFS_ERR_H
#define _HSFS_ERR_H

#include <errno.h>
#include <assert.h>

#include <hsfs.h>

extern struct hsfs_inode bad_hsfs_inode;

#define MAX_ERRNO (sizeof(bad_hsfs_inode))

#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)-MAX_ERRNO)

static inline void * ERR_PTR(long error)
{
	unsigned long tmp = (long)&bad_hsfs_inode;

	if (error < 0)
		tmp -= error;
	else
		tmp -= error;

	assert(tmp < (unsigned long)(&bad_hsfs_inode + 1));
	return (void *)tmp;
}

static inline long PTR_ERR(const void *ptr)
{
	return (long) ptr - (long)&bad_hsfs_inode;
}

static inline long IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

static inline long IS_ERR_OR_NULL(const void *ptr)
{
	return !ptr || IS_ERR_VALUE((unsigned long)ptr);
}

#if 0
/**
 * ERR_CAST - Explicitly cast an error-valued pointer to another pointer type
 * @ptr: The pointer to cast.
 *
 * Explicitly cast an error-valued pointer to another pointer type in such a
 * way as to make it clear that's what's going on.
 */
static inline void * __must_check ERR_CAST(const void *ptr)
{
	/* cast away the const */
	return (void *) ptr;
}

static inline int __must_check PTR_RET(const void *ptr)
{
	if (IS_ERR(ptr))
		return PTR_ERR(ptr);
	else
		return 0;
}

#endif

#endif /* _LINUX_ERR_H */
