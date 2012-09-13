#include <errno.h>
#include "hsi_nfs3.h"

int hsi_nfs3_stat2sattr(struct stat *st, int to_set, 
			struct hsfs_sattr *attr)
{
  	int err = 0;
  	
	DEBUG_IN("%s", "\n");

	if (!st) {
		err = EINVAL; 
		ERR("hsi_nfs3_stat2sattr func 1st input param invalid.\n");
		goto out;
	}
	if (!attr) {
		err = EINVAL; 
		ERR("hsi_nfs3_stat2sattr func 2nd input param invalid.\n");
		goto out;
	}
	attr->valid = to_set & 0x0fff; /* may have some problems */
	attr->mode = st->st_mode & 0777777;
	attr->uid = st->st_uid;
	attr->gid = st->st_gid;
	attr->size = st->st_size;
	attr->atime.tv_sec = st->st_atime;
	attr->mtime.tv_sec = st->st_mtime;
#if defined __USE_MISC || defined __USE_XOPEN2K8
	attr->atime.tv_nsec = st->st_atim.tv_nsec;
	attr->mtime.tv_nsec = st->st_mtim.tv_nsec;
#else
	attr->atime.tv_nsec = st->st_atimensec;
	attr->mtime.tv_nsec = st->st_mtimensec;
#endif

 out:
	DEBUG_OUT("with errno : %d.\n", err);
	return err;
}
