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
#include "hsx_fuse.h"

void hsx_fuse_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr, 
		      int to_set, struct fuse_file_info *fi __attribute__((unused)))
{
  	int err = 0;
	double to = 0;
	struct stat st;
	struct hsfs_iattr sattr;
	struct hsfs_inode *inode = NULL;
	struct hsfs_super *sb = NULL;
	
	DEBUG_IN("%s", "\n");

	sb = (struct hsfs_super *) fuse_req_userdata(req);
	if (NULL == sb) {
		err = EINVAL;
		ERR("req is invalid.\n");
		goto out;
	}

	inode = hsfs_ilookup(sb, ino);
	if (NULL == inode) {
		err = ENOENT;
		ERR("ino :%lu is invalid.\n", ino);
		goto out;
	}

	hsx_fuse_stat2iattr(attr, to_set, &sattr);

	err = hsfs_ll_setattr(inode, &sattr);
	if (err)
		goto out;

	hsfs_generic_fillattr(inode, &st);
 out:
	DEBUG_OUT("ino : %lu with errno : %d.\n", inode->ino, err);
	if (err)
		fuse_reply_err(req, err);	
	else {
		to = S_ISDIR(inode->i_mode) ? sb->acdirmin : sb->acregmin;
		fuse_reply_attr(req, &st, to);
	}
}
