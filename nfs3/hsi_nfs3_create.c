#ifdef HSFS_NFS3_TEST
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/vfs.h>
#include <libgen.h>
#endif

#include "hsi_nfs3.h"
#include "hsfs.h"
#include "nfs3.h"
#include "log.h"

#define FULL_MODE (S_IRWXU | S_IRWXG | S_IRWXO)

int hsi_nfs3_create(struct hsfs_inode *hi, struct hsfs_inode **newhi,
		const char *name, mode_t mode)
{
	struct create3args args;
	struct diropres3 res;
	mode_t cmode = mode & 0xF0000;
	mode_t fmode = mode & FULL_MODE;
	CLIENT *clntp = NULL;
	int status = 0;

	DEBUG_IN("%s%d", "In hsi_fuse_create(), with MODE = ", mode);
	
	memset(&args, 0, sizeof(struct create3args));
	memset(&res, 0, sizeof(struct diropres3));
	
	args.where.dir = hi->fh;
	args.where.name = (char *)name;
	clntp = hi->sb->clntp;
	args.how.mode = cmode >> 16;
	if (EXCLUSIVE == args.how.mode) {
		memcpy(args.how.createhow3_u.verf, &hi->ino, NFS3_CREATEVERFSIZE);
	} else {
		struct sattr3 sattr;
		memset(&sattr, 0, sizeof(struct sattr3));
		sattr.mode.set = 1;
		sattr.mode.set_uint32_u.val = fmode;
		sattr.uid.set = 1;
		sattr.uid.set_uint32_u.val = hi->attr.uid;
		sattr.gid.set = 1;
		sattr.gid.set_uint32_u.val = hi->attr.gid;
		sattr.size.set = 0;
		sattr.atime.set = SET_TO_SERVER_TIME;
		sattr.mtime.set	= SET_TO_SERVER_TIME;

		args.how.createhow3_u.obj_attributes = sattr;
	}
	
	status = hsi_nfs3_clnt_call(hi->sb, NFSPROC3_CREATE,
			(xdrproc_t)xdr_create3args, (caddr_t)&args,
			(xdrproc_t)xdr_diropres3, (caddr_t)&res);
	if (status)
		goto out;

	if (NFS3_OK == res.status) {
		status = res.status;

		*newhi = hsi_nfs3_ifind(hi->sb, 
			&res.diropres3_u.resok.obj.post_op_fh3_u.handle, 
			&res.diropres3_u.resok.
			obj_attributes.post_op_attr_u.attributes);
	
		if (EXCLUSIVE == args.how.mode) {
			struct hsfs_sattr sattr = {};
			sattr.mode = fmode;
			S_SETMODE(&sattr);
			status = hsi_nfs3_setattr(*newhi, &sattr);
			if (status) {
				WARNING("Setattr failed after create: %d", status);
			}
		}
	} else {
		status = hsi_nfs3_stat_to_errno(res.status);
	}

out:
	clnt_freeres(clntp, (xdrproc_t)xdr_diropres3, (caddr_t)&res);
	DEBUG_OUT("%s%d", "Out of hsi_nfs3_create(), with STATUS = ", status);
	return status;
	
}

#ifdef HSFS_NFS3_TEST
int main(int argc, char *argv[])
{
	struct hsfs_inode hi;
	struct hsfs_super hs;
	struct hsfs_inode *newhi;
	char *cliname = NULL;
	char *svraddr = NULL;
	CLIENT *mclntp = NULL;
	char * fpath = NULL;
	int mode = 0;

	if (argc < 3) {
		printf("MISSING args in running program access!\n");
		goto out;
	}
	
	newhi = (struct hsfs_inode *)malloc(sizeof(struct hsfs_inode));
	memset(&hi, 0, sizeof(struct hsfs_inode));
	memset(&hs, 0, sizeof(struct hsfs_super));
	hi.sb = &hs;
	
	cliname = basename(argv[0]);
	svraddr = argv[1];
	fpath = argv[2];
	mclntp = clnt_create(svraddr, NFS_PROGRAM, NFS_V3, "TCP");
	if (NULL == mclntp) {
	printf("ERROR occur while executing <clnt_create> process\n"    );
		goto out;
	}
	hi.sb->clntp = mclntp;
	hi.attr.uid = getuid();
	hi.attr.gid = getgid();
	size_t fhlen;
	unsigned char *fhvalp;
	map_path_to_nfs3fh(svraddr, fpath, &fhlen, &fhvalp);
	hi.fh.data.data_len = fhlen;
	hi.fh.data.data_val = fhvalp;
	printf("result:%d\n",hsi_nfs3_create(&hi, &newhi, "test", 0x201FF));

	return 0;

out:
	printf("Hello, this is out!\n");
}
#endif /*end of HSFS_NFS3_TEST*/

