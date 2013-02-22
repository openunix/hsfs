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
#include <errno.h>

#include <libmemcached/memcached.h>

#include <hsfs.h>


const char *config_string= "--SERVER=127.0.0.1";

int hsi_mem_start(int argc, char **argv, struct hsfs_super *sb)
{
	int ret = 0;

	DEBUG_IN("(%d, %p, %p)", argc, argv, sb);
	memcached_st *memc= memcached(config_string, strlen(config_string));	
	if (memc == NULL){
		ret = ENOMEM;
		goto out;
	}
	

	DEBUG_OUT("with memcached_st at %p", memc);
	exit(1);
out:
	return ret;
}
