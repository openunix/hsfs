HSFS_NFS3_X = nfs3.x
HSFS_NFS3_H = nfs3.h
HSFS_NFS3_XDR = nfs3xdr.c
HSFS_NFS3_CLNT = nfs3clnt.c

HSFS_NFS3_FILE = $(HSFS_NFS3_H) $(HSFS_NFS3_XDR) $(HSFS_NFS3_CLNT)

noinst_PROGRAMS = hsfs

hsfs_LDFLAGS = -ltirpc `pkg-config fuse --libs`
hsfs_CPPFLAGS = $(AM_CFLAGS) -g -Wall -I/usr/include/tirpc/ \
		`pkg-config fuse --cflags`
hsfs_SOURCES = nfs3xdr.c nfs3clnt.c hsfs.c

$(HSFS_NFS3_H) : $(HSFS_NFS3_X)
	rpcgen -h -o $@ $<

$(HSFS_NFS3_XDR) : $(HSFS_NFS3_X) $(HSFS_NFS3_H)
	rpcgen -c -o $@ $<

$(HSFS_NFS3_CLNT) : $(HSFS_NFS3_X) $(HSFS_NFS3_H)
	rpcgen -l -o $@ $<

CLEANFILES = $(HSFS_NFS3_FILE)
