#
# Makefile for ENFS
#

ifneq ($(KERNELRELEASE),)	# Kbuild makefile

# There is no clean way to pass "TOPDIR" to kbuild. But
# kbuild sets "src" to the relative dirname of this
# makefile, allowing us to infer "TOPDIR" here.
#
# "TOPDIR" is renamed, to avoid potential conflict with
# kbuild.
bwfs_topdir := $(src)/../..

obj-m += enfs.o
enfs-objs := dir.o file.o inode.o nfs2xdr.o pagelist.o proc.o read.o \
             symlink.o unlink.o write.o nfs3proc.o nfs3xdr.o direct.o

EXTRA_CFLAGS := -I. -I$(bwfs_topdir)/xdbg -I$(bwfs_topdir)/2.6/vdmap

# RHEL 4 AS Update 4
ifneq ($(findstring 2.6.9-42.EL,$(KERNELRELEASE)),)
# "__DIR_F_POS_INDEX" enables the patch that improves the semantics
# of file offsets of directory files. If defined, "f_pos" fields of
# directory files will represent indexes of directory entries,
# rather than those implementation-defined server-generated
# cookies.
#
# This change is found in later kernels. We might wanna incorporate
# it for them too. Thus, a new macro dedicate to this change is
# introduced. Whereas, "__AS44_SUP" is reserved for version-specific
# stuff.
EXTRA_CFLAGS += -D__AS44_SUP -D__DIR_F_POS_INDEX
endif

else	# BWFS makefile

KERNELDIR ?= /lib/modules/$(shell uname -r)/build

.PHONY: all
all:
	$(MAKE) -C $(KERNELDIR) M=`pwd` modules

.PHONY: clean
clean:
	$(MAKE) -C $(KERNELDIR) M=`pwd` clean

.PHONY: install
install:
	$(MAKE) -C $(KERNELDIR) M=`pwd` INSTALL_MOD_PATH=$(INSTALL_DIR) modules_install

endif
