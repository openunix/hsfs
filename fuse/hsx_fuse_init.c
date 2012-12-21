/*
 * Copyright (C) 2012 Feng Shuo <steve.shuo.feng@gmail.com>
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
#include <hsx_fuse.h>

void hsx_fuse_init(void *userdata, struct fuse_conn_info *conn)
{
	struct hsfs_super *sb = (struct hsfs_super *)userdata;
	char unsupported[1024] = "";
	int len;
	unsigned long ref;

	DEBUG_IN("SB(%p), P_VER(%d.%d)", sb,
		 conn->proto_major, conn->proto_minor);

	ref = hsx_fuse_ref_xchg(sb->root, 0);
	FUSE_ASSERT(ref == 0);

#ifdef FUSE_CAP_BIG_WRITES
	if (conn->capable & FUSE_CAP_BIG_WRITES)
		conn->want |= FUSE_CAP_BIG_WRITES;
	else
		strcat(unsupported, "FUSE_CAP_BIG_WRITES, ");
#endif	/* FUSE_CAP_BIG_WRITES */



	len = strlen(unsupported);
	if (len){
		unsupported[len - 2] = '\0';
		WARNING("Your fuse has %s builtin while your kernel not.");
	}
	
	DEBUG_OUT("Success conn at %p", conn);
}
