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

int hsfs_init_icache(struct hsfs_super *sb)
{
	hash_init(sb->id_table, HSFS_ID_HASH_SIZE);
	hash_init(sb->id_table, HSFS_NAME_HASH_SIZE);
}

struct hsfs_inode *hsfs_ilookup(struct hsfs_super *sb, uint64_t ino)
{
	return __hsfs_ilookup(sb, ino);
}
