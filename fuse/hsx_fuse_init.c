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


/* Fuse 2.7 don't have capable/want */
#if FUSE_VERSION > 27
#define CHECK_CAP(xx) do {				\
		if (conn->capable & xx)			\
			conn->want |= xx;		\
		else					\
			strcat(unsupported, #xx", ");	\
	}while(0)

static void hsx_fuse_init_cap(struct hsfs_super *sb, struct fuse_conn_info *conn)
{
	char unsupported[1024] = "";
	int len;
	
	DEBUG_IN("Capable(0x%x)", conn->capable);
	(void)sb;	  /* Stop warning for some configuration... */
	
#ifdef FUSE_CAP_BIG_WRITES
	CHECK_CAP(FUSE_CAP_BIG_WRITES);
#endif
#ifdef FUSE_CAP_AUTO_INVAL_DATA
	CHECK_CAP(FUSE_CAP_AUTO_INVAL_DATA);
#endif
#ifdef FUSE_CAP_READDIR_PLUS
	CHECK_CAP(FUSE_CAP_READDIR_PLUS);
#endif
	
	len = strlen(unsupported);
	if (len){
		unsupported[len - 2] = '\0';
		WARNING("Your fuse has %s builtin while your kernel not.", unsupported);
	}
	
	DEBUG_OUT("Want(0x%x)", conn->capable);
}
#elif FUSE_VERSION == 27
static void hsx_fuse_init_cap(struct hsfs_super *sb, struct fuse_conn_info *conn)
{
	(void)sb;
	(void)conn;
}
#else
#error "Need FUSE version 2.7 or higher to work."
#endif	/* FUSE_VERSION */

void hsx_fuse_init(void *userdata, struct fuse_conn_info *conn)
{
	struct hsfs_super *sb = (struct hsfs_super *)userdata;
	unsigned long ref;

	DEBUG_IN("SB(%p), Kernel_VER(%d.%d), FUSE_VER(%d.%d)", sb,
		 conn->proto_major, conn->proto_minor,
		 FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION);

	ref = hsx_fuse_ref_xchg(sb->root, 1);
	FUSE_ASSERT(ref == 0);

	hsx_fuse_init_cap(sb, conn);

	DEBUG_OUT("Success conn at %p", conn);
}
