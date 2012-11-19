/*
 * Copyright (C) 2012 Shou Xiaoyun, Feng Shuo <steve.shuo.feng@gmail.com>
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

#include "hsfs.h"
#include "hsx_fuse.h"

void hsx_fuse_forget(fuse_req_t req, fuse_ino_t ino, unsigned long nlookup)
{
	struct hsfs_inode  *hsfs_node;
	struct hsfs_super  *sb;
	unsigned long ilookup;
	
	DEBUG_IN("(%p, %lu, %lu)", req, (unsigned long)ino, nlookup);

	sb = fuse_req_userdata(req);	

	hsfs_node = hsfs_ilookup(sb, ino);
	assert(hsfs_node != NULL);

	ilookup = (unsigned long)hsfs_node->private;
	DEBUG("FUSE Inode(%lu) nlookup %lu to forget %lu.", ino, ilookup, nlookup);
	assert(ilookup >= nlookup);

	ilookup -= nlookup;
	hsfs_node->private = (void *)ilookup;

	if(!ilookup)
		hsfs_iput(hsfs_node);

	DEBUG_OUT("rest lookup %lu", ilookup);
	fuse_reply_none(req);
}
