HSFS_NFS3_X = nfs3.x
HSFS_NFS3_H = nfs3.h
HSFS_NFS3_XDR = nfs3xdr.c
HSFS_NFS3_CLNT = nfs3clnt.c

HSFS_MNT_X = mount.x
HSFS_MNT_H = mount.h
HSFS_MNT_XDR = mount_xdr.c
HSFS_MNT_CLNT = mount_clnt.c

HSFS_NFS3_FILES = $(HSFS_NFS3_H) $(HSFS_NFS3_XDR) $(HSFS_NFS3_CLNT)
HSFS_MNT_FILES = $(HSFS_MNT_H) $(HSFS_MNT_XDR) $(HSFS_MNT_CLNT)

noinst_PROGRAMS = hsfs

hsfs_LDFLAGS = -ltirpc `pkg-config fuse --libs`
hsfs_CPPFLAGS = $(AM_CFLAGS) -g -Wall -I/usr/include/tirpc/ \
		`pkg-config fuse --cflags`
hsfs_SOURCES = $(HSFS_NFS3_XDR) $(HSFS_NFS3_CLNT) $(HSFS_MNT_XDR) \
	       $(HSFS_MNT_CLNT) xcommon.c hsfsmount.c hsfs.c

$(HSFS_NFS3_H) : $(HSFS_NFS3_X)
	rpcgen -h -o $@ $<

$(HSFS_NFS3_XDR) : $(HSFS_NFS3_X) $(HSFS_NFS3_H)
	rpcgen -c -o $@ $<

$(HSFS_NFS3_CLNT) : $(HSFS_NFS3_X) $(HSFS_NFS3_H)
	rpcgen -l -o $@ $<

$(HSFS_MNT_H) : $(HSFS_MNT_X)
	rpcgen -h -o $@ $<

$(HSFS_MNT_XDR) : $(HSFS_MNT_X) $(HSFS_MNT_H)
	rpcgen -c -o $@ $<

$(HSFS_MNT_CLNT) : $(HSFS_MNT_X) $(HSFS_MNT_H)
	rpcgen -l -o $@ $<

CLEANFILES = $(HSFS_NFS3_FILES) $(HSFS_MNT_FILES)