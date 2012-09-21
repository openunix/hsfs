HSFS_NFS3_X = nfs3.x
HSFS_NFS3_H = $(top_builddir)/include/nfs3.h
HSFS_NFS3_XDR = nfs3xdr.c

HSFS_MNT_X = mount.x
HSFS_MNT_H = $(top_builddir)/include/mount.h
HSFS_MNT_XDR = mountxdr.c

HSFS_ACL3_X = acl3.x
HSFS_ACL3_H = $(top_builddir)/include/acl3.h
HSFS_ACL3_XDR = acl3xdr.c

HSFS_NFS3_FILES = $(HSFS_NFS3_H) $(HSFS_NFS3_XDR)
HSFS_MNT_FILES = $(HSFS_MNT_H) $(HSFS_MNT_XDR)
HSFS_ACL3_FILES = $(HSFS_ACL3_H) $(HSFS_ACL3_XDR)

noinst_LIBRARIES = libhsi_nfs3.a

libhsi_nfs3_a_CPPFLAGS = $(AM_CFLAGS) -g -Wall -I/usr/include/tirpc/ \
			 `pkg-config fuse --cflags`
libhsi_nfs3_a_SOURCES = $(HSFS_NFS3_XDR) $(HSFS_MNT_XDR) $(HSFS_ACL3_XDR) \
			fstab.c nfs_mntent.c hsi_nfs3_mount.c \
			hsi_nfs3_fattr2stat.c hsi_nfs3_unlink.c hsi_nfs3_getattr.c \
			hsi_nfs3_fsinfo.c hsi_nfs3_pathconf.c hsi_nfs3_statfs.c\
			hsi_nfs3_lookup.c hsi_nfs3_mkdir.c hsi_nfs3_read.c \
			hsi_nfs3_write.c hsi_nfs3_stat2sattr.c hsi_nfs3_setattr.c \
			hsi_nfs3_rmdir.c hsi_nfs3_readlink.c hsi_nfs3_symlink.c \
			hsi_nfs3_rename.c hsi_nfs3_readdir.c \
			hsi_nfs3_mknod.c  hsi_nfs3_link.c hsi_nfs3_create.c \
			hsi_nfs3_access.c hsi_nfs3_getxattr.c hsi_acl3.c \
			hsi_nfs3_setxattr.c

$(HSFS_NFS3_H) : $(HSFS_NFS3_X)
	test -f $@ && rm -rf $@ || true
	rpcgen -h -o $@ $<

$(HSFS_NFS3_XDR) : $(HSFS_NFS3_X) $(HSFS_NFS3_H)
	test -f $@ && rm -rf $@ || true
	rpcgen -c -C -o $@ $<

$(HSFS_MNT_H) : $(HSFS_MNT_X)
	test -f $@ && rm -rf $@ || true
	rpcgen -h -o $@ $<

$(HSFS_MNT_XDR) : $(HSFS_MNT_X) $(HSFS_MNT_H)
	test -f $@ && rm -rf $@ || true
	rpcgen -c -C -o $@ $<

$(HSFS_ACL3_H) : $(HSFS_ACL3_X)
	test -f $@ && rm -rf $@ || true
	rpcgen -h -o $@ $<

$(HSFS_ACL3_XDR) : $(HSFS_ACL3_X) $(HSFS_ACL3_H)
	test -f $@ && rm -rf $@ || true
	rpcgen -c -C -o $@ $<

CLEANFILES = $(HSFS_NFS3_FILES) $(HSFS_MNT_FILES) $(HSFS_ACL3_FILES)
