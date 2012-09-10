
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
/* #include <fuse/fuse_lowlevel.h>*/
#include "hsfs.h"
#include "nfs3.h"
#include "log.h"

/* Defined in fuse/fuse_lowlevel.h */
#ifndef FUSE_SET_ATTR_MODE 
#define FUSE_SET_ATTR_MODE       (1 << 0)
#endif

#ifndef FUSE_SET_ATTR_UID
#define FUSE_SET_ATTR_UID        (1 << 1)
#endif

#ifndef FUSE_SET_ATTR_GID
#define FUSE_SET_ATTR_GID        (1 << 2)
#endif

#ifndef FUSE_SET_ATTR_SIZE
#define FUSE_SET_ATTR_SIZE       (1 << 3)
#endif

#ifndef FUSE_SET_ATTR_ATIME
#define FUSE_SET_ATTR_ATIME      (1 << 4)
#endif

#ifndef FUSE_SET_ATTR_MTIME
#define FUSE_SET_ATTR_MTIME      (1 << 5)
#endif

#ifndef FUSE_SET_ATTR_ATIME_NOW
#define FUSE_SET_ATTR_ATIME_NOW  (1 << 7)
#endif

#ifndef FUSE_SET_ATTR_MTIME_NOW
#define FUSE_SET_ATTR_MTIME_NOW  (1 << 8)
#endif

#define S_ISSETMODE(to_set)       ((to_set) & FUSE_SET_ATTR_MODE)
#define S_ISSETUID(to_set)        ((to_set) & FUSE_SET_ATTR_UID)
#define S_ISSETGID(to_set)        ((to_set) & FUSE_SET_ATTR_GID)
#define S_ISSETSIZE(to_set)       ((to_set) & FUSE_SET_ATTR_SIZE)
#define S_ISSETATIME(to_set)      ((to_set) & FUSE_SET_ATTR_ATIME)
#define S_ISSETMTIME(to_set)      ((to_set) & FUSE_SET_ATTR_MTIME)
#define S_ISSETATIMENOW(to_set)   ((to_set) & FUSE_SET_ATTR_ATIME_NOW)
#define S_ISSETMTIMENOW(to_set)   ((to_set) & FUSE_SET_ATTR_MTIME_NOW)

int hsi_nfs3_stat_to_errno(int status);
int hsi_rpc_stat_to_errno(CLIENT *clntp);

int hsi_nfs3_setattr(struct hsfs_inode *inode, struct hsfs_sattr *attr){
	int err = 0, i;
	CLIENT *clntp;
	struct timeval to = {120, 0};
	/*struct setattr3args {
	  nfs_fh3 object;
	  sattr3 new_attributes;
	  sattrguard3 guard;
	};
	*/
	struct setattr3args args;

	/*struct wccstat3 {
	  nfsstat3 status;
	  union {
	    wcc_data wcc;
	  } wccstat3_u;
	};
	*/
	struct wccstat3 res;
	enum clnt_stat st;
  
	INFO("Enter hsi_nfs3_setattr().\n");

	clntp= inode->sb->clntp;
	memset(&args, 0, sizeof(args));
	args.object.data.data_len = inode->fh.data.data_len;
        args.object.data.data_val = inode->fh.data.data_val;
	/*check and print filehandle*/
	/* fprintf(stdout, "Filehandle : Len=%lu, Content=",args.object.data.data_len); */
	fprintf(stdout, "Filehandle : Len=%u, Content=",args.object.data.data_len); 
	//INFO("Filehandle : Len=%lu, Content=", args.object.data.data_len);
	for (i=0; i < args.object.data.data_len; i++)
		/* fprintf(stdout, "%02x", (unsigned char) (args.object.data.data_val[i])); */
		//INFO("%02x", (unsigned char) (args.object.data.data_val[i]));
		fprintf(stdout, "%02x", (unsigned char) (args.object.data.data_val[i]));
	if (args.object.data.data_len > 0)
		/* fprintf(stdout, "\n"); */
		//INFO("\n");
		fprintf(stdout, "\n");
	memset(&res, 0, sizeof(res));
	if (S_ISSETMODE(attr->valid)) {
		args.new_attributes.mode.set = TRUE;
		args.new_attributes.mode.set_uint32_u.val = attr->mode;
		fprintf(stdout, "we are in modify mode, new mode is %lo (octal).\n", attr->mode);
	}
	else {
		fprintf(stdout, "we are not in modify mode.\n");
		args.new_attributes.mode.set = FALSE;
	}	
	       
	if (S_ISSETUID(attr->valid)) {
		args.new_attributes.uid.set = TRUE;
		args.new_attributes.uid.set_uint32_u.val = attr->uid;
		fprintf(stdout, "we are in modify uid, new uid is %lu.\n", attr->uid);
	}
	else {
	  args.new_attributes.uid.set = FALSE;
	  fprintf(stdout, "we are not in modify uid.\n");
	}


	if (S_ISSETGID(attr->valid)) {
		args.new_attributes.gid.set = TRUE;
		args.new_attributes.gid.set_uint32_u.val = attr->gid;
		fprintf(stdout, "we are in modify gid, new gid is %lu.\n", attr->gid);
        }
        else {
		args.new_attributes.gid.set = FALSE;
		fprintf(stdout, "we are not in modify gid.\n");
	}

	if (S_ISSETSIZE(attr->valid)) {
		fprintf(stdout, "we are in modify size, size is %llu.\n", attr->size);
		args.new_attributes.size.set = TRUE;
		args.new_attributes.size.set_uint64_u.val = attr->size;
        }
        else{
		args.new_attributes.size.set = FALSE;
		fprintf(stdout, "we are not in modify size.\n");
	}
	if (S_ISSETATIME(attr->valid)) {
		args.new_attributes.atime.set = SET_TO_CLIENT_TIME;
		args.new_attributes.atime.set_time_u.time.seconds = attr->atime.tv_sec;
		args.new_attributes.atime.set_time_u.time.nseconds = attr->atime.tv_nsec;
		fprintf(stdout, "we are in modify atime, new atime is %lu.\n", attr->atime.tv_sec);
        }
	else {
		args.new_attributes.atime.set = DONT_CHANGE;
		fprintf(stdout, "we are not in modify atime.\n");
	}
	
	if (S_ISSETMTIME(attr->valid)) {
        	args.new_attributes.mtime.set = SET_TO_CLIENT_TIME;
		args.new_attributes.mtime.set_time_u.time.seconds = attr->mtime.tv_sec;
		args.new_attributes.mtime.set_time_u.time.nseconds = attr->mtime.tv_nsec;
		fprintf(stdout, "we are in modify mtime, new mtime is %lu.\n", attr->mtime.tv_sec);
        }
        else {
		args.new_attributes.mtime.set = DONT_CHANGE;
		fprintf(stdout, "we are not in modify mtime.\n");
	}
	if (S_ISSETATIMENOW(attr->valid)) { 
		args.new_attributes.atime.set = SET_TO_SERVER_TIME;
		fprintf(stdout, "we are in modify atime now.\n");
	}

	if (S_ISSETMTIMENOW(attr->valid)) {
		args.new_attributes.mtime.set = SET_TO_SERVER_TIME;
		fprintf(stdout, "we are in modify mtime now.\n");
        }
	
	args.guard.check = FALSE;

	st = clnt_call(clntp, 2, (xdrproc_t)xdr_setattr3args, (caddr_t)&args, 
		       (xdrproc_t)xdr_wccstat3, (caddr_t)&res, to);
	if (st) {
		err = hsi_rpc_stat_to_errno(clntp);
	  	/* fprintf(stderr, "Call RPC Server failure: %d.\n", err); */
		//ERR("Call RPC Server failure: %d.\n", err);
		fprintf(stderr, "Call RPC Server failure: %d.\n", err); 
		goto out;
	}
	if (NFS3_OK != res.status) {
		err = hsi_nfs3_stat_to_errno(res.status);//return negative number
		/* fprintf(stderr, "return value status incorrect, error : %d.\n", err); */
		//ERR("return value status incorrect, error : %d.\n", err);
		fprintf(stderr, "return value status incorrect, error : %d.\n", err);
		goto out;
	}
	if (res.wccstat3_u.wcc.after.present) {
		inode->attr.type = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.type;
		inode->attr.mode = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.mode;
		inode->attr.nlink = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.nlink;
		inode->attr.uid = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.uid;
		inode->attr.gid = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.gid;
		inode->attr.size = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.size;
		inode->attr.used = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.used;
		inode->attr.rdev.major = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.rdev.major;
		inode->attr.rdev.minor = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.rdev.minor;
		inode->attr.fsid = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.fsid;
		inode->attr.fileid = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.fileid;
		
		inode->attr.atime.seconds = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.atime.seconds;
		inode->attr.atime.nseconds = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.atime.nseconds;
		inode->attr.mtime.seconds = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.mtime.seconds;
		inode->attr.mtime.nseconds = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.mtime.nseconds;
		inode->attr.ctime.seconds = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.ctime.seconds;
		inode->attr.ctime.nseconds = res.wccstat3_u.wcc.after.post_op_attr_u.attributes.ctime.nseconds;
	}
	else {
	        err = 11; /* errno : EAGAIN */
		//ERR("Try again to set file attributes.\n");
		fprintf(stderr, "Try again to set file attributes.\n"); 
		goto out;
	}	
 out:
	INFO("Leave hsi_nfs3_setattr() with errno : %d.\n", err);
	return err;
}
