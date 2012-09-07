bin_PROGRAMS = hsfs
hsfs_CPPFLAGS = $(AM_CFLAGS) -g -Wall -I/usr/include/tirpc/ \
		`pkg-config fuse --cflags` 
hsfs_LDFLAGS = `pkg-config fuse --libs`
hsfs_LDADD = $(top_builddir)/nfs3/libhsi_nfs3.a \
	     $(top_builddir)/fuse/libhsx_fuse.a
hsfs_SOURCES = xcommon.c hsfs_main.c

noinst_LIBRARIES = libhsx_fuse.a
libhsx_fuse_a_CPPFLAGS = $(AM_CFLAGS) -g -Wall -I/usr/include/tirpc/ \
			`pkg-config fuse --cflags`
libhsx_fuse_a_SOURCES = hsfs_fuse_table.c  
