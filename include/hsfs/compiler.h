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

#ifndef _HSFS_COMPILER_H_
#define _HSFS_COMPILER_H_

/* To pull in config.h */
#include <hsfs/types.h>

static inline int fls(int x)
{
	return (sizeof(x) * 8 - __builtin_clz(x));
}

static inline int fls64(long long x)
{
	return (sizeof(x) * 8 - __builtin_clzll(x));
}

static inline int __ffs(int x)
{
	return __builtin_ffs(x);
}	

#endif	/* _HSFS_COMPILER_H_ */
