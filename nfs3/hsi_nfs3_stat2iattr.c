/*
 * Copyright (C) 2012 Yan Huan, Feng Shuo <steve.shuo.feng@gmail.com>
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

#include <errno.h>
#include "hsi_nfs3.h"

/* XXX Should move to hsfs/ or remove it at all */
#include "../fuse/fuse_misc.h"

int hsi_nfs3_stat2iattr(struct stat *st, int to_set, struct hsfs_iattr *attr)
{
  	int err = 0;
  	
	DEBUG_IN("%s", "\n");

	if (!st) {
		err = EINVAL; 
		ERR("hsi_nfs3_stat2sattr func 1st input param invalid.\n");
		goto out;
	}
	if (!attr) {
		err = EINVAL; 
		ERR("hsi_nfs3_stat2sattr func 2nd input param invalid.\n");
		goto out;
	}
	attr->valid = to_set & 0x0fff; /* may have some problems */
	attr->mode = st->st_mode & 0777777;
	attr->uid = st->st_uid;
	attr->gid = st->st_gid;
	attr->size = st->st_size;

	attr->atime.tv_sec = st->st_atime;
	attr->mtime.tv_sec = st->st_mtime;
	attr->ctime.tv_sec = st->st_ctime;
	attr->atime.tv_nsec = ST_ATIM_NSEC(st);
	attr->mtime.tv_nsec = ST_MTIM_NSEC(st);
	attr->ctime.tv_nsec = ST_CTIM_NSEC(st);

 out:
	DEBUG_OUT("with errno : %d.\n", err);
	return err;
}
