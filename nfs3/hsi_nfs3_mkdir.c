#include <hsfs/err.h>
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

struct hsfs_inode *hsi_nfs3_handle_create(struct hsfs_super *sb, struct diropres3ok *dir)
{
	struct nfs_fattr fattr;
	struct nfs_fh fh;
	struct nfs_fh3 *nfh;
	
	nfs_init_fattr(&fattr);
	hsi_nfs3_post2fattr(&(dir->obj_attributes), &fattr);

	/* Actually, we should do lookup here..... */
	if (!dir->obj.present)
		return ERR_PTR(ENOENT);
	
	nfh = &(dir->obj.post_op_fh3_u.handle);
	nfs_copy_fh3(&fh, nfh->data.data_len, nfh->data.data_val);

	return hsi_nfs_fhget(sb, &fh, &fattr);
}

int hsi_nfs3_mkdir (struct hsfs_inode *parent, struct hsfs_inode **new,
	       		const char *name, mode_t mode)
{
	struct hsfs_super *sb = parent->sb;
	int err = 0;
	mkdir3args argp;
	diropres3 clnt_res;
	
	DEBUG_IN(" ino: %lu.\n", parent->ino);
	memset(&argp, 0, sizeof(mkdir3args));
	memset(&clnt_res, 0, sizeof(diropres3));

	hsi_nfs3_getfh3(parent, &argp.where.dir);

	argp.where.name = (char *)name;
	argp.attributes.mode.set = 1;
	argp.attributes.mode.set_uint32_u.val = mode&0xfff;
		
	err = hsi_nfs3_clnt_call (sb, sb->clntp, NFSPROC3_MKDIR, 
			(xdrproc_t) xdr_mkdir3args, (caddr_t) &argp,
		       	(xdrproc_t) xdr_diropres3, (caddr_t) &clnt_res);
	if (err) {	/*RPC error*/
		*new = NULL;
		goto out;
	}
	if (0 != clnt_res.status) {	/*RPC is OK, nfs error*/
		*new = NULL;
		err = hsi_nfs3_stat_to_errno(clnt_res.status);
		goto outfree;
	}

	*new = hsi_nfs3_handle_create(sb, &clnt_res.diropres3_u.resok);
	if(IS_ERR(*new)){
		*new = NULL;
		err = PTR_ERR(*new);
	}

outfree:
	clnt_freeres(sb->clntp, (xdrproc_t)xdr_diropres3, (char *)&clnt_res);
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
	st = hsi_nfs3_mkdir(parent, &new, name, 555);
	if (st) {
		err = EIO;
		fprintf (stderr, "%s: Call RPC Server (%s, %u, %u) failure: (%s).\n", cliname, svraddr, NFSPROC3_MKDIR, NFS_V3,clnt_sperrno(st));
		goto out;
	}
	if (0 == st){
		printf("Create ok!\n");
	}

out:
	free(parent->sb);
	free(parent);
	if (NULL != clntp)
		clnt_destroy(clntp);
	exit(st);
}
#endif /* TEST  */

