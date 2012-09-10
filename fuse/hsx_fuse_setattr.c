
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

void hsx_fuse_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr, 
		      int to_set, struct fuse_file_info *fi){
  	int err = 0;
	struct stat st;
	struct hsfs_sattr sattr;
	struct hsfs_inode *inode = NULL;
	struct hsfs_super *sb = NULL;
	
	DEBUG_IN("Enter hsx_fuse_setattr().\n");

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
	err = hsi_nfs3_stat2attr(attr, to_set, &sattr);
	if (err) {
		DEBUG_OUT("Leave hsx_fuse_setattr() with errno : %d.\n", err);
		fuse_reply_err(req, err);
	}
	else
		err = hsi_nfs3_setattr(inode, &sattr);
 out:
	if (err) {
		DEBUG_OUT("Leave hsx_fuse_setattr() with errno : %d.\n", err);
		fuse_reply_err(req, err);
	}
	else {
		err = hsi_nfs3_fattr2stat(&inode->attr, &st);
		DEBUG_OUT("Leave hsx_fuse_setattr() with errno : %d.\n", err);
		if (err)
			fuse_reply_err(req, err);
		else
			fuse_reply_attr(req, &st, inode->sb->timeo * 10);
	}
}
