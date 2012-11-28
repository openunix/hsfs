/*
 * Copyright (C) 2012 Yan Huan, Feng Shuo <steve.shuo.feng@gmail.com>
 *
 * This file is part of HSFS.
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

#include <errno.h>

#include "hsx_fuse.h"
#include "fuse_misc.h"

void hsx_fuse_stat2iattr(struct stat *st, int to_set, struct hsfs_iattr *attr)
{
	DEBUG_IN("(ST:%p, SET:%x, IATTR:%p)", st, to_set, attr);

	attr->valid = 0;
	if (to_set & FUSE_SET_ATTR_MODE){
		attr->valid |= HSFS_ATTR_MODE;
		attr->mode = st->st_mode & ~S_IFMT; /* Do we need this? */
	}
	if (to_set & FUSE_SET_ATTR_UID){
		attr->valid |= HSFS_ATTR_UID;
		attr->uid = st->st_uid;
	}
	if (to_set & FUSE_SET_ATTR_GID){
		attr->valid |= HSFS_ATTR_GID;
		attr->gid = st->st_gid;
	}
	if (to_set & FUSE_SET_ATTR_SIZE){
		attr->valid |= HSFS_ATTR_SIZE;
		attr->size = st->st_size;
	}

	/* 
	 * The following is most weird part......
	 */
	if (to_set & FUSE_SET_ATTR_ATIME){
		attr->atime.tv_sec = st->st_atime;
		attr->atime.tv_nsec = ST_ATIM_NSEC(st);
		attr->valid |= HSFS_ATTR_ATIME;
		if (!(to_set & FUSE_SET_ATTR_ATIME_NOW))
			attr->valid |= HSFS_ATTR_ATIME_SET; 
	}
	if (to_set & FUSE_SET_ATTR_MTIME){
		attr->mtime.tv_sec = st->st_mtime;
		attr->mtime.tv_nsec = ST_MTIM_NSEC(st);
		attr->valid |= HSFS_ATTR_MTIME;
		if (!(to_set & FUSE_SET_ATTR_MTIME_NOW))
			attr->valid |= HSFS_ATTR_MTIME_SET; 
	}
#if 0
	/* Do we have ctime? */
	attr->ctime.tv_sec = st->st_ctime;
	attr->ctime.tv_nsec = ST_CTIM_NSEC(st);
#endif

	DEBUG_OUT("(iattr->valid:%x)", attr->valid);
}
