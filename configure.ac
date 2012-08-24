#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([hsfs], [0.0.1], [steve.shuo.feng@gmail.com])
AC_CONFIG_SRCDIR([fuse/hsfs_main.c])
AC_CONFIG_HEADERS([include/config.h])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([fuse], [fuse_lowlevel_new], [have_fuse=yes], [have_fuse=no])
if test "x$have_fuse" = xno; then
   echo "Can't find libfuse, you need fuse-devel to compile me."
   exit 1
fi


# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h libintl.h limits.h memory.h mntent.h netdb.h netinet/in.h paths.h stdlib.h string.h sys/mount.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UID_T

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([alarm gethostbyname hasmntopt inet_ntoa memset realpath socket strchr strcspn strdup strerror strrchr strstr uname])

AC_CONFIG_FILES([Makefile
                 fuse/Makefile])
AC_OUTPUT
