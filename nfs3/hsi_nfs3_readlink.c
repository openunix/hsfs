/*
 * hsi_nfs3_readlink
 * xuehaitao
 * 2012.9.6
 */

#ifdef HSFS_NFS3_TEST
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <rpc/rpc.h>
#include <libgen.h>
#include "nfs3.h"
#include "apis.h"
#include "log.h"
#include "hsfs.h"
#include "hsi_nfs3.h"

char *cliname = NULL;

int main(int argc, char *argv[])
{
	char *svraddr = "10.10.19.124";
	char *fpath = NULL;
	size_t fh_len;
	unsigned char *fh_val = NULL;
	CLIENT *clntp = NULL;
	int result = 0;
	int err = 0;
	int read_stat = 0;
	int reply_stat = 0;
	const  char * nfs_link = NULL;
	char contents[10];
	nfs_link = contents;
	struct hsfs_inode symlink ;
	struct hsfs_super sb;
	cliname = basename(argv[0]);

	if (argc < 2) {
		err = EINVAL;
		fprintf(stderr, "%s $svraddr $fpath.\n", cliname);
		goto out;
	} 	
	fpath = argv[1];
	result = map_path_to_nfs3fh(svraddr, fpath, &fh_len, &fh_val);
	if(result != 0){
		fprintf(stderr, "The transfer is failed!err is %d.\n ",result);
		err = result;
		goto out;
	}

	clntp = clnt_create(svraddr, NFS_PROGRAM, NFS_V3, "TCP");
	if (clntp == NULL){
		err = ENXIO;
		goto out;	
	}

	memset(&symlink, 0, sizeof(symlink));
	memset(&sb, 0, sizeof(sb));
	symlink.sb = &sb;
	symlink.sb->clntp = clntp;
	symlink.fh.data.data_len = fh_len;
	symlink.fh.data.data_val = fh_val;

	read_stat= hsi_nfs3_readlink(&symlink, &nfs_link);
	if(read_stat){
		fprintf(stderr, "read_stat = %d\n", read_stat);
		goto out;
	}
out:
	return err;
}
#endif

#include "nfs3.h"
#include "hsi_nfs3.h"
#include "log.h"


int hsi_nfs3_readlink(struct hsfs_inode *hi, char **nfs_link)
{
	struct readlink3res res;
	enum clnt_stat st;
	struct nfs_fh3 fh_readlink;
	CLIENT *clntp = NULL;
	struct timeval to;
	int err = 0;
	int len = 0;

	to.tv_sec = hi->sb->timeo / 10;
	to.tv_usec = (hi->sb->timeo % 10) * 100000;
	memset(&res, 0, sizeof(res));
	memset(&fh_readlink, 0, sizeof(fh_readlink));
	fh_readlink = hi->fh;
	clntp = hi->sb->clntp;
	DEBUG_IN("%s\n", "hsi_nfs3_readlink");

	st = clnt_call(clntp, NFSPROC3_READLINK, (xdrproc_t)xdr_nfs_fh3,
			(caddr_t)&fh_readlink, (xdrproc_t)xdr_readlink3res,
			(caddr_t)&res, to);

	if(st){
		ERR("call the RPC server fail %s\n", clnt_sperrno(st));
		err = hsi_rpc_stat_to_errno(clntp);
		goto out2;
	}
	st = res.status;
	if(NFS3_OK != st){
		ERR("the proc of readlink  is failed %d\n", st);
		err = hsi_nfs3_stat_to_errno(st);
		goto out1;
	}
	len = strlen(res.readlink3res_u.resok.data);
	*nfs_link = (char *)malloc(len+1);
	if((*nfs_link) == NULL){
		goto out1;
	}
	strcpy(*nfs_link, res.readlink3res_u.resok.data);
out1:
	clnt_freeres(clntp, (xdrproc_t)xdr_readlink3res, (char *)&res);
out2:
	DEBUG_OUT("the hsi_nfs3_readlink return with errno.%d\n", err);
	return err;
}

