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

int hsi_nfs3_rmdir (struct hsfs_inode *parent, const char *name)
{
	struct hsfs_super *sb = parent->sb;
	int err = 0;
	diropargs3 argp;
	wccstat3 clnt_res;

	DEBUG_IN(" ino:%lu.\n", parent->ino);

	memset (&argp, 0, sizeof(diropargs3));
	memset (&clnt_res, 0, sizeof(wccstat3));
		
	argp.dir.data.data_len = parent->fh.data.data_len;
	argp.dir.data.data_val = parent->fh.data.data_val;
	argp.name = (char *) name;
		
	err = hsi_nfs3_clnt_call (sb, sb->clntp, NFSPROC3_RMDIR, 
			(xdrproc_t) xdr_diropargs3, (caddr_t) &argp,
			 (xdrproc_t) xdr_wccstat3, (caddr_t) &clnt_res);
		
	if (0 != err)
		goto out;
	
	err = hsi_nfs3_stat_to_errno(clnt_res.status); 	/*nfs error.*/
	clnt_freeres(sb->clntp, (xdrproc_t)xdr_wccstat3, (char *)&clnt_res);
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
	struct hsfs_inode *parent = NULL ;
	struct hsfs_inode *new;
	cliname = basename (argv[0]);
	if (argc < 3) {
			err = EINVAL;
			fprintf(stderr, "%s $svraddr $fpath.\n", cliname);
			goto out;
		}
	parent = (struct hsfs_inode *) malloc(sizeof(struct hsfs_inode));
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
	parent->sb = (struct hsfs_super *) malloc(sizeof(struct hsfs_super));
	if (NULL == parent->sb)
	{
		printf("No memory:parent->sb.\n");
	}
	parent->sb->clntp = clntp;
	parent->fh.data.data_len = args.where.dir.data.data_len;
	parent->fh.data.data_val = args.where.dir.data.data_val;
	st = hsi_nfs3_rmdir(parent, name);
	if (st) {
		err = EIO;
		fprintf (stderr, "%s: Call RPC Server (%s, %u, %u) failure: (%s).\n", cliname, svraddr, NFSPROC3_MKDIR, NFS_V3,clnt_sperrno(st));
		goto out;
	}
	if (0 == st){
		printf("Rm ok!\n");
	}

out:
	free(parent->sb);
	free(parent);
	if (NULL != clntp)
		clnt_destroy(clntp);
	exit(st);
}
#endif 

