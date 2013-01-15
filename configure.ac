#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.60])
AC_INIT([hsfs], [0.3], [steve.shuo.feng@gmail.com])
AC_CONFIG_SRCDIR([fuse/hsfs_main.c])
AC_CONFIG_HEADERS([include/config.h])

AM_INIT_AUTOMAKE

# SLES 11SP2 (automake-1.10.1) doesn't have this
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Cygwin put tirpc into /usr/include/tirpc.
AC_SUBST([TIRPC_HEADERS])
AC_ARG_WITH([tirpc-headers],
	[AC_HELP_STRING([--with-tirpc-headers=<path>], [Alternative path to tirpc headers])],
	[TIRPC_HEADERS="-I $withval"
	case $BWLABELDIR in
     	     /*) ;;
     	     *) BWLABELDIR="`pwd`/$BWLABELDIR" ;;
	esac])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O

AC_GNU_SOURCE

AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB([fuse], [fuse_lowlevel_new], [], AC_MSG_ERROR([You need libfuse to run.]))
AC_CHECK_LIB([tirpc], [clnt_tp_create], [], AC_MSG_ERROR([You need libtirpc to run.]))

# Checks for header files.
AC_HEADER_ASSERT
AC_HEADER_DIRENT
AC_HEADER_MAJOR
AC_HEADER_RESOLV
AC_HEADER_STAT
AC_HEADER_STDBOOL

AC_CHECK_HEADERS([arpa/inet.h fcntl.h libintl.h limits.h memory.h mntent.h netdb.h netinet/in.h paths.h stdlib.h string.h sys/mount.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for types.
AC_TYPE_INT32_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UID_T


# Checks for structures
AC_CHECK_MEMBERS([struct stat.st_atim])
AC_CHECK_MEMBERS([struct stat.st_atimespec])

# Check for compiler characteristics.
AC_C_INLINE

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([alarm gethostbyname hasmntopt inet_ntoa memset realpath socket strchr strcspn strdup strerror strrchr strstr uname])

# Check for system service
AC_SYS_LARGEFILE

AC_CONFIG_FILES([Makefile
                 fuse/Makefile
                 nfs3/Makefile
                 man/Makefile
		 lib/Makefile
		 nfs_common/Makefile
		])
AC_OUTPUT
