CFLAGS += -Wall -I../include

.PHONY: all
all: mount.enfs

.PHONY: clean
clean:
	rm -f mount.enfs mount.h mount_xdr.c *.o

mount.h: mount.x
	rpcgen -h -o $@ $^

mount_xdr.c: mount.x
	rpcgen -c -o $@ $^

mount.enfs: mount.o nfsmount.o nfsumount.o mount_xdr.o \
	   xcommon.o fstab.o nfs_mntent.o conn.o
	$(CC) -o $@ $^

mount.o: mount.h
