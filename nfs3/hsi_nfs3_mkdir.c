/**
 * hsi_nfs3_mkdir.c
 * Hu yuwei
 * 2012/09/06
 */

#include "hsi_nfs3.h"
#ifdef HSFS_NFS3_TEST
#include "hsi_nfs3.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <libgen.h>
#include <string.h>
#endif

int hsi_nfs3_mkdir (struct hsfs_inode *hi_parent, struct hsfs_inode **hi_new,
	       		const char *name, mode_t mode)
{
	int err = 0;
	mkdir3args argp;
	diropres3 clnt_res;	
	
	DEBUG_IN(" ino: %lu.\n", hi_parent->ino);
	memset(&argp, 0, sizeof(mkdir3args));
	memset(&clnt_res, 0, sizeof(diropres3));
	argp.where.dir.data.data_len = hi_parent->fh.data.data_len;
	argp.where.dir.data.data_val = hi_parent->fh.data.data_val;
	argp.where.name = (char *)name;
	argp.attributes.mode.set = 1;
	argp.attributes.mode.set_uint32_u.val = mode&0xfff;
		
	err = hsi_nfs3_clnt_call (hi_parent->sb, NFSPROC3_MKDIR, 
			(xdrproc_t) xdr_mkdir3args, (caddr_t) &argp,
		       	(xdrproc_t) xdr_diropres3, (caddr_t) &clnt_res);
	if (err) {	/*RPC error*/
		*hi_new = NULL;
		goto out;
	}
	if (0 != clnt_res.status) {	/*RPC is OK, nfs error*/
		*hi_new = NULL;
		err = hsi_nfs3_stat_to_errno(clnt_res.status);
		goto outfree;
	}
	*hi_new = hsi_nfs3_ifind (hi_parent->sb,
			&(clnt_res.diropres3_u.resok.obj.post_op_fh3_u.handle),
	&(clnt_res.diropres3_u.resok.obj_attributes.post_op_attr_u.attributes));
	if(NULL == *hi_new) {
		ERR("Error in create inode.\n");
	}

outfree:
	clnt_freeres(hi_parent->sb->clntp, (xdrproc_t)xdr_diropres3,
		       	(char *)&clnt_res);
out:
	DEBUG_OUT(" out, errno:%d\n", err);
	return err;
};
#ifdef HSFS_NFS3_TEST
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
	st = hsi_nfs3_mkdir(hi_parent, &new, name, 555);
	if (st) {
		err = EIO;
		fprintf (stderr, "%s: Call RPC Server (%s, %u, %u) failure: (%s).\n", cliname, svraddr, NFSPROC3_MKDIR, NFS_V3,clnt_sperrno(st));
		goto out;
	}
	if (0 == st){
		printf("Create ok!\n");
	}

out:
	free(hi_parent->sb);
	free(hi_parent);
	if (NULL != clntp)
		clnt_destroy(clntp);
	exit(st);
}
#endif /* TEST  */

