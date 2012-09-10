
#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE 
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <libgen.h>
#include <fuse/fuse_lowlevel.h>
#include "hsfs.h"
#include "hsx_fuse.h"
#include "hsi_nfs3.h"
#include "log.h"

void hsx_fuse_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi){
  	int err = 0;
	struct stat st;
	struct hsfs_inode *inode = NULL;
	struct hsfs_super *sb = NULL;
	
	DEBUG_IN("%s", "Enter hsx_fuse_getattr().\n");

	sb = (struct hsfs_super *) fuse_req_userdata(req);
	if (NULL == sb) {
	  err = 22; /* errno : EINVAL*/
	  ERR("req is invalid.\n");
	  goto out;
        }
	inode = hsx_fuse_iget(sb, ino);
	if (NULL == inode) {
		err = 22; /* errno : EINVAL*/
		ERR("ino :%lu is invalid.\n", ino);
		goto out;
	}
	err = hsi_nfs3_getattr(inode);
 out:
	if (err) {
		DEBUG_OUT("Leave hsx_fuse_getattr() with errno : %d.\n", err);
		fuse_reply_err(req, err);
	}
	else {
		err = hsi_nfs3_fattr2stat(&inode->attr, &st);
		DEBUG_OUT("Leave hsx_fuse_getattr() with errno : %d.\n", err);
		if (err)
			fuse_reply_err(req, err);
		else
			fuse_reply_attr(req, &st, inode->sb->timeo * 10);
	}
}
