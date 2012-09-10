
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
#include "hsfs.h"
#include "log.h"

int hsi_nfs3_stat2sattr(struct stat *st, int to_set, 
			struct hsfs_sattr *attr){
  	int err = 0;
  	/* ASSERT(st);
	ASSERT(attr);
	*/

  	INFO("Enter hsi_nfs3_stat2sattr().\n");

	if (!st) {
		err = 22; /*errno : EINVAL */
		ERR("hsi_nfs3_stat2sattr func 1st input param invalid.\n");
		goto out;
	}
	if (!attr) {
          	err = 22; /*errno : EINVAL */
		ERR("hsi_nfs3_stat2sattr func 2nd input param invalid.\n");
		goto out;
        }
	attr->valid = to_set & 0x0fff; /* may have some problems */
	attr->mode = st->st_mode & 0777777;
	attr->uid = st->st_uid;
	attr->gid = st->st_gid;
	attr->size = st->st_size;
	attr->atime.tv_sec = st->st_atime;
	attr->atime.tv_nsec = 0;
	attr->mtime.tv_sec = st->st_mtime;
	attr->mtime.tv_nsec = 0;
 out:
	INFO("Leave hsi_nfs3_stat2sattr() with errno : %d.\n", err);
	return err;
}
