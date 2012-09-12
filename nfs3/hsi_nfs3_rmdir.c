/**
 * hsi_nfs3_rmdir.c
 * Hu yuwei
 * 2012/09/06
 */
#include "hsi_nfs3.h"
#include "hsx_fuse.h"

#ifdef HSFS_NFS3_TEST
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <libgen.h>
#endif

int hsi_nfs3_rmdir (struct hsfs_inode *hi_parent, const char *name)
{
	int err = 0;
	diropargs3 argp;
	wccstat3 clnt_res;
	struct timeval timeout = { hi_parent->sb->timeo/10,
	       	(hi_parent->sb->timeo%10)*100000 };
	DEBUG_IN(" in\n","hsi_nfs3_rmdir");

	memset (&argp, 0, sizeof(diropargs3));
	memset (&clnt_res, 0, sizeof(wccstat3));
		
	argp.dir.data.data_len = hi_parent->fh.data.data_len;
	argp.dir.data.data_val = hi_parent->fh.data.data_val;
	argp.name = (char *) name;
		
	err = clnt_call (hi_parent->sb->clntp, NFSPROC3_RMDIR, 
			(xdrproc_t) xdr_diropargs3, (caddr_t) &argp,
			 (xdrproc_t) xdr_wccstat3, (caddr_t) &clnt_res,
			       	timeout);
		
	if (0 != err) {					/*err is a RPC clnt error. So use hsi_rpc_stat_to_errno().*/
			err = hsi_rpc_stat_to_errno(hi_parent->sb->clntp);
			goto out;
		}
	
	err = clnt_res.status;
	
	if (NFS3_OK != err) {
			err = hsi_nfs3_stat_to_errno(clnt_res.status); 	
			goto out;
		}
	err = hsi_nfs3_stat_to_errno(clnt_res.status); 	/*nfs error.*/
	clnt_freeres(hi_parent->sb->clntp, (xdrproc_t)xdr_wccstat3,
		       	(char *)&clnt_res);
out:
	DEBUG_OUT(" out, errno is(%d)\n", err);
	return err;
};
#ifdef 	HSFS_NFS3_TEST
char *cliname = NULL;
int main(int argc, char *argv[])
{	
	char *svraddr = NULL;
	CLIENT *clntp = NULL;
	char *fpath = NULL;
	char *name = NULL;
	struct mkdir3args args;
	struct diropres3 res;
	/*struct fattr *fattrp = NULL;*/
	struct timeval to = {120, 0};
	enum clnt_stat st;
	int err = 0;
	int argv_len = 0;
	struct hsfs_inode *hi_parent = NULL ;
	struct hsfs_inode *new;
	cliname = basename (argv[0]);
	if (argc < 3) {
			err = EINVAL;
			fprintf(stderr, "%s $svraddr $fpath.\n", cliname);
			goto out;
		}
	hi_parent = (struct hsfs_inode *) malloc(sizeof(struct hsfs_inode));
	svraddr = argv[1];
	fpath = argv[2];
	name = argv[3];
	clntp = clnt_create(svraddr, 100003, NFS_V3, "tcp");
	if (NULL == clntp)
	{
		fprintf(stderr, "%s: Create handle to RPC server (%s, %u, %u) failed: (%s).\n", cliname,svraddr, NFSPROC3_MKDIR, NFS_V3, clnt_spcreateerror(cliname));
		err = ENXIO;
		goto out;
	} 
	memset (&args, 0, sizeof(args));
	memset (&res, 0, sizeof(res));
	args.where.name = fpath;
	//args.path.fpath_len = strlen(fpath);
	int st_tmp;
	size_t fh_len = 0;
	unsigned char *fh;
	st_tmp = map_path_to_nfs3fh(svraddr, fpath, &fh_len, &fh);
	args.where.dir.data.data_len  = fh_len;
	args.where.dir.data.data_val = fh;
	hi_parent->sb = (struct hsfs_super *) malloc(sizeof(struct hsfs_super));
	if (NULL == hi_parent->sb)
	{
		printf("No memory:hi_parent->sb.\n");
	}
	hi_parent->sb->clntp = clntp;
	hi_parent->fh.data.data_len = args.where.dir.data.data_len;
	hi_parent->fh.data.data_val = args.where.dir.data.data_val;
	st = hsi_nfs3_rmdir(hi_parent, name);
	if (st) {
		err = EIO;
		fprintf (stderr, "%s: Call RPC Server (%s, %u, %u) failure: (%s).\n", cliname, svraddr, NFSPROC3_MKDIR, NFS_V3,clnt_sperrno(st));
		goto out;
	}
	if (0 == st){
		printf("Rm ok!\n");
	}

out:
	free(hi_parent->sb);
	free(hi_parent);
	if (NULL != clntp)
		clnt_destroy(clntp);
	exit(st);
}
#endif 

