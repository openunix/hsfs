/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 *	Copyright 1994,2001-2003 Sun Microsystems, Inc.
 *	All rights reserved.
 *	Use is subject to license terms.
 */

/*
 * ident	"%Z%%M%	%I%	%E% SMI"
 */

%#include "nfs3.h"
const NFS_ACL_MAX_ENTRIES = 1024;

typedef int uid;
typedef unsigned short o_mode;

/*
 * This is the format of an ACL which is passed over the network.
 */
struct aclent {
	int type;
	uid id;
	o_mode perm;
};

/*
 * The values for the type element of the aclent structure.
 */
const NA_USER_OBJ = 0x1;	/* object owner */
const NA_USER = 0x2;		/* additional users */
const NA_GROUP_OBJ = 0x4;	/* owning group of the object */
const NA_GROUP = 0x8;		/* additional groups */
const NA_CLASS_OBJ = 0x10;	/* file group class and mask entry */
const NA_OTHER_OBJ = 0x20;	/* other entry for the object */
const NA_ACL_DEFAULT = 0x1000;	/* default flag */

/*
 * The bit field values for the perm element of the aclent
 * structure.  The three values can be combined to form any
 * of the 8 combinations.
 */
const NA_READ = 0x4;		/* read permission */
const NA_WRITE = 0x2;		/* write permission */
const NA_EXEC = 0x1;		/* exec permission */

/*
 * This is the structure which contains the ACL entries for a
 * particular entity.  It contains the ACL entries which apply
 * to this object plus any default ACL entries which are
 * inherited by its children.
 *
 * The values for the mask field are defined below.
 */
struct secattr {
	u_int mask;
	int aclcnt;
	aclent aclent<NFS_ACL_MAX_ENTRIES>;
	int dfaclcnt;
	aclent dfaclent<NFS_ACL_MAX_ENTRIES>;
};

/*
 * The values for the mask element of the secattr struct as well
 * as for the mask element in the arguments in the GETACL2 and
 * GETACL3 procedures.
 */
const NA_ACL = 0x1;		/* aclent contains a valid list */
const NA_ACLCNT = 0x2;		/* the number of entries in the aclent list */
const NA_DFACL = 0x4;		/* dfaclent contains a valid list */
const NA_DFACLCNT = 0x8;	/* the number of entries in the dfaclent list */



/*
 * This is the definition for the GETACL procedure which applies
 * to NFS Version 3 files.
 */
struct GETACL3args {
	nfs_fh3 fh;
	u_int mask;
};

struct GETACL3resok {
	post_op_attr attr;
	secattr acl;
};

struct GETACL3resfail {
	post_op_attr attr;
};

union GETACL3res switch (nfsstat3 status) {
case NFS3_OK:
	GETACL3resok resok;
default:
	GETACL3resfail resfail;
};

/*
 * This is the definition for the SETACL procedure which applies
 * to NFS Version 3 files.
 */
struct SETACL3args {
	nfs_fh3 fh;
	secattr acl;
};

struct SETACL3resok {
	post_op_attr attr;
};

struct SETACL3resfail {
	post_op_attr attr;
};

union SETACL3res switch (nfsstat3 status) {
case NFS3_OK:
	SETACL3resok resok;
default:
	SETACL3resfail resfail;
};

/*
 * This is the definition for the GETXATTRDIR procedure which applies
 * to NFS Version 3 files.
 */
struct GETXATTRDIR3args {
	nfs_fh3 fh;
	bool create;
};

struct GETXATTRDIR3resok {
	nfs_fh3 fh;
	post_op_attr attr;
};

union GETXATTRDIR3res switch (nfsstat3 status) {
case NFS3_OK:
	GETXATTRDIR3resok resok;
default:
	void;
};


/* XXX } */

/*
 * Share the port with the NFS service.  NFS has to be running
 * in order for this service to be useful anyway.
 */
const NFS_ACL_PORT = 2049;

/*
 * This is the definition for the ACL network protocol which is used
 * to provide support for Solaris ACLs for files which are accessed
 * via NFS Version 2 and NFS Version 3.
 */
program NFS_ACL_PROGRAM {
	version NFS_ACL_V3 {
		void
		 ACLPROC3_NULL(void) = 0;
		GETACL3res
		 ACLPROC3_GETACL(GETACL3args) = 1;
		SETACL3res
		 ACLPROC3_SETACL(SETACL3args) = 2;
		GETXATTRDIR3res
		 ACLPROC3_GETXATTRDIR(GETXATTRDIR3args) = 3;
	} = 3;
} = 100227;
