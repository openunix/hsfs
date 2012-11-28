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

#ifndef _HSFS_TYPES_H_
#define _HSFS_TYPES_H_

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif	/* HAVE_CONFIG_H */

#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif	/* HAVE_INTTYPES_H */

/* By C99, stdint.h should be in inttypes.h already */
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif	/* HAVE_STDINT_H */

/* We need this for mode_t and others */
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#endif	/* HAVE_STDBOOL_H */

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#ifdef __CYGWIN__
# include <bits/wordsize.h>
#endif	/* __CYGWIN__ */

#define BITS_PER_LONG __WORDSIZE

/* #ifndef HAVE_BOOL */
/* typedef enum { */
/* 	false = 0, */
/* 	true = 1 */
/* }bool; */
/* #endif	/\* HAVE_BOOL *\/ */
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8, __u8;

typedef int32_t __s32;

#define __u64 u64
#define __u32 u32
#define __u16 u16

#if defined MAJOR_IN_MKDEV
# include <sys/mkdev.h>
#elif defined MAJOR_IN_SYSMACROS
# include <sys/sysmacros.h>
#endif



#endif	/* _HSFS_TYPES_H_ */
