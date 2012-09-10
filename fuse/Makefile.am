bin_PROGRAMS = hsfs
hsfs_CPPFLAGS = $(AM_CFLAGS) -g -Wall -I/usr/include/tirpc/ \
		`pkg-config fuse --cflags` 
hsfs_LDFLAGS = `pkg-config fuse --libs`
hsfs_LDADD = $(top_builddir)/fuse/libhsx_fuse.a \
	     $(top_builddir)/nfs3/libhsi_nfs3.a
hsfs_SOURCES = xcommon.c hsfs_main.c

noinst_LIBRARIES = libhsx_fuse.a
libhsx_fuse_a_CPPFLAGS = $(AM_CFLAGS) -g -Wall -I/usr/include/tirpc/ \
			`pkg-config fuse --cflags`
libhsx_fuse_a_SOURCES = hsfs_fuse_table.c hsx_fuse_getattr.c hsx_fuse_statfs.c \
			hsx_fuse_lookup.c hsx_fuse_fill_reply.c hsx_fuse_open.c	hsx_fuse_release.c

