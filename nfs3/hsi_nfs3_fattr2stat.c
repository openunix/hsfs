#include <errno.h>
#include "hsi_nfs3.h"

int hsi_nfs3_fattr2stat(struct fattr3 *attr, struct stat *st)
{
	int err = 0;
	unsigned int type = 0;

	DEBUG_IN("%s", "Enter hsi_nfs3_fattr2stat().\n");

	st->st_dev = makedev(0, 0); /* ignored now */
	st->st_ino = (ino_t) (attr->fileid);/*depend on 'use_ino' mount option*/
	type = (unsigned int) (attr->type);
	st->st_mode = (mode_t) (attr->mode & 07777);
	switch (type) {
		case NF3REG :
			st->st_mode |= S_IFREG;
			break;
		case NF3DIR :
			st->st_mode |= S_IFDIR;
			break;
		case NF3BLK :
			st->st_mode |= S_IFBLK;
			break;
		case NF3CHR :
			st->st_mode |= S_IFCHR;
			break;
       		case NF3LNK :
			st->st_mode |= S_IFLNK;
			break;
       		case NF3SOCK :
			st->st_mode |= S_IFSOCK;
			break;
		case NF3FIFO :
			st->st_mode |= S_IFIFO;
			break;
		default :
		  	err = EINVAL;
			goto out;
	}
	st->st_nlink = (nlink_t) (attr->nlink);
	st->st_uid = (uid_t) (attr->uid);
	st->st_gid = (gid_t) (attr->gid);
	st->st_rdev = makedev(attr->rdev.major, attr->rdev.minor);
	st->st_size = (off_t) (attr->size);
	st->st_blksize = 0; /* ignored now */
	st->st_blocks = (blkcnt_t) (attr->used / 512);
	st->st_atime = (time_t) (attr->atime.seconds);
	st->st_mtime = (time_t) (attr->mtime.seconds);
	st->st_ctime = (time_t) (attr->ctime.seconds);
	#if defined __USE_MISC || defined __USE_XOPEN2K8
		st->st_atim.tv_nsec = attr->atime.nseconds;           
		st->st_mtim.tv_nsec = attr->mtime.nseconds;           
		st->st_ctim.tv_nsec = attr->ctime.nseconds;           
	#else
		st->st_atimensec = attr->atime.nseconds;  
		st->st_mtimensec = attr->mtime.nseconds;  
		st->st_ctimensec = attr->ctime.nseconds;  
	#endif
 out:
	DEBUG_OUT("Leave hsi_nfs3_fattr2stat() with errno %d.\n", err);
	return err;
}
