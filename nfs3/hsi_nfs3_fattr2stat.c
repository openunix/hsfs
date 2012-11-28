/*
 * Copyright (C) 2012 Zhao Yan, Feng Shuo <steve.shuo.feng@gmail.com>
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
#include "hsi_nfs3.h"
#include "../fuse/fuse_misc.h"

int hsi_nfs3_fattr2stat(struct fattr3 *attr, struct stat *st)
{
	int err = 0;
	unsigned int type = 0;

	DEBUG_IN("%s", "\n");

	st->st_dev = makedev(0, 0); /* ignored now */
	st->st_ino = (ino_t) (attr->fileid);/*depend on 'use_ino' mount option*/
	type = (unsigned int) (attr->type);
	st->st_mode = (mode_t) (attr->mode & 07777);
	switch (type) {
		case NF3REG :
			st->st_mode |= S_IFREG;
			break;
		case NF3DIR :
			st->st_mode |= S_IFDIR;
			break;
		case NF3BLK :
			st->st_mode |= S_IFBLK;
			break;
		case NF3CHR :
			st->st_mode |= S_IFCHR;
			break;
		case NF3LNK :
			st->st_mode |= S_IFLNK;
			break;
		case NF3SOCK :
			st->st_mode |= S_IFSOCK;
			break;
		case NF3FIFO :
			st->st_mode |= S_IFIFO;
			break;
		default :
		  	err = EINVAL;
			goto out;
	}
	st->st_nlink = (nlink_t) (attr->nlink);
	st->st_uid = (uid_t) (attr->uid);
	st->st_gid = (gid_t) (attr->gid);
	st->st_rdev = makedev(attr->rdev.major, attr->rdev.minor);
	st->st_size = (off_t) (attr->size);
	st->st_blksize = 0; /* ignored now */
	st->st_blocks = (blkcnt_t) (attr->used / 512);
	st->st_atime = (time_t) (attr->atime.seconds);
	st->st_mtime = (time_t) (attr->mtime.seconds);
	st->st_ctime = (time_t) (attr->ctime.seconds);

	ST_ATIM_NSEC_SET(st, attr->atime.nseconds);
	ST_MTIM_NSEC_SET(st, attr->atime.nseconds);
	/* Don't we need ctime? */

 out:
	DEBUG_OUT("with errno %d.\n", err);
	return err;
}
