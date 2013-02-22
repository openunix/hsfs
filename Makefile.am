include $(top_srcdir)/Makefile.common

if ENABLE_MEMCACHED
 MEMCACHED_DIR = memcached
else
 MEMCACHED_DIR =
endif

SUBDIRS = nfs3 nfs_common lib fuse man $(MEMCACHED_DIR)
