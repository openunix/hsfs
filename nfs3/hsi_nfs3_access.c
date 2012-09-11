#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/vfs.h>
#include <libgen.h>

#include "hsi_nfs3.h"
#include "log.h"
#include "nfs3.h"

int hsi_nfs3_access(struct hsfs_inode *hi, int mask)
{
	DEBUG_IN("%s", "We now come into hsi_nfs3_access()\n");
	struct access3args args;
	struct access3res res;
	struct timeval to = {120, 0};
	enum clnt_stat st;
	int status = 0;

	memset(&args, 0, sizeof(struct access3args));
	memset(&res, 0, sizeof(struct access3res));
	args.object = hi->fh;
	args.access = mask & 0x3F;
	
	st = clnt_call(hi->sb->clntp, NFSPROC3_ACCESS,
		(xdrproc_t)xdr_access3args, (caddr_t)&args,
		(xdrproc_t)xdr_access3res, (caddr_t)&res, to);
	if (st) {
		printf("ERROR occur while executing <clnt_call()> process!\n");
		status = hsi_rpc_stat_to_errno(hi->sb->clntp);
		goto out;
	}
	
	if (NFS3_OK == res.status) {
		status = res.status;
	} else {
		status = hsi_nfs3_stat_to_errno(res.status);
	}

out:
DEBUG_OUT("%s", "Out of hsi_nfs3_access()\n");
	return res.status;
}

#ifdef HSFS_NFS3_TEST
int main(int argc, char *argv[])
{
	struct hsfs_inode hi;
	struct hsfs_super hs;
	char *cliname = NULL;
	char *svraddr = NULL;
	CLIENT *mclntp = NULL;
	char * fpath = NULL;
	int mask = 0;

	if (argc < 3) {
		printf("MISSING args in running program access!\n");
		//goto out;
	}
	memset(&hi, 0, sizeof(struct hsfs_inode));
	memset(&hs, 0, sizeof(struct hsfs_super));
	hi.sb = &hs;
	cliname = basename(argv[0]);
	svraddr = argv[1];
	fpath = argv[2];
	mclntp = clnt_create(svraddr, NFS_PROGRAM, NFS_V3, "TCP");
	perror("4");
	if (NULL == mclntp) {
		printf("ERROR occur while executing <clnt_create> process\n");
		goto out;
	}
	hi.sb->clntp = mclntp;

	size_t fhlen;
	unsigned char *fhvalp;
	map_path_to_nfs3fh(svraddr, fpath, &fhlen, &fhvalp);
	hi.fh.data.data_len = fhlen;
	hi.fh.data.data_val = fhvalp;
	//mask = 0x1;
	mask = 0x3F;
	while (mask & 0x3F) {
		printf("Result for %x:%d\n", mask, hsi_nfs3_access(&hi, mask));
		mask = mask >> 1;

	}
	return 0;
out:
	perror("Hello, this is out!\n");
}
#endif
