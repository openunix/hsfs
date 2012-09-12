/**
 * hsi_nfs3_symlink.c
 * xuehaitao
 *2012.9.6
 **/


#ifdef HSFS_NFS3_TEST

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/vfs.h>
#include <rpc/rpc.h>
#include <libgen.h>
#include "nfs3.h"
#include "apis.h"
#include "hsfs.h"
#include "log.h"
#include "hsi_nfs3.h"

char *cliname = NULL;
int main(int argc, char *argv[])
{
	char *svraddr = "10.10.19.124";
        char *fpath = "/home/xue/";
        size_t fh_len;
        unsigned char *fh_val = NULL;
        CLIENT *clntp = NULL;
        int result = 0;
        int err = 0;
        int symlink_stat = 0;
	const char *nfs_link = NULL;
	const char *nfs_name = NULL;
	struct hsfs_super sb_parent;
	struct hsfs_super *sb_new;
	struct hsfs_inode parent;
        struct hsfs_inode *new = NULL;
	
	cliname = basename(argv[0]);

        if (argc < 3) {
                err = EINVAL;
                fprintf(stderr, "%s $nfs_link $nfs_name.\n", cliname);
                goto out;
        }
        nfs_link = argv[1];
        nfs_name = argv[2];

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
        new = (struct hsfs_inode*)malloc(sizeof(struct hsfs_inode));
	sb_new = (struct hsfs_super*)malloc(sizeof(struct hsfs_super));
       	memset(&sb_parent, 0, sizeof(struct hsfs_super));
        new->sb = sb_new;
	parent.sb = &sb_parent;
	parent.sb->clntp = clntp;
        parent.fh.data.data_len = fh_len;
        parent.fh.data.data_val = fh_val;

	symlink_stat = hsi_nfs3_symlink(&parent, &new, nfs_link, nfs_name);

	if(symlink_stat){
                err = EACCES;
                fprintf(stderr, "read_stat = %d\n", symlink_stat);
                goto out;
        }
out:
        free(new);
	free(sb_new);
	return err;
}

#endif

#include "nfs3.h"
#include "log.h"
#include "hsi_nfs3.h"

int hsi_nfs3_symlink(struct hsfs_inode *parent, struct hsfs_inode **new,
                     const char *nfs_link, const char *nfs_name)
{
	struct symlink3args args;
	struct diropres3 res;
	enum clnt_stat st;
	struct timeval to;
	int err = 0;
	DEBUG_IN("%s\n", "hsi_nfs3_symlink");

	to.tv_sec = parent->sb->timeo / 10;
	to.tv_usec = (parent->sb->timeo % 10) * 100000;
	memset(&res, 0, sizeof(res));
	memset(&args, 0, sizeof(args));
	args.where.dir = parent->fh;
	args.where.name = (char *)nfs_name;
	args.symlink.symlink_data = (char *)nfs_link;

	st = clnt_call(parent->sb->clntp, NFSPROC3_SYMLINK,
			(xdrproc_t)xdr_symlink3args,
			(caddr_t)&args, (xdrproc_t)xdr_diropres3,
			(caddr_t)&res, to);
	if(st){
		ERR( " Call RPC Server failure:%s.\n ", clnt_sperrno(st));
		err = hsi_rpc_stat_to_errno(parent->sb->clntp);
		goto out2;
	}
	st = res.status;
	if(NFS3_OK != st){
		ERR("the proc of symlink failure:%d\n", st);
		err = hsi_nfs3_stat_to_errno(st);
		goto out1;
	}
	*new = hsi_nfs3_ifind(parent->sb, 
	&(res.diropres3_u.resok.obj.post_op_fh3_u.handle),
	&(res.diropres3_u.resok.obj_attributes.post_op_attr_u.attributes));
out1:
	clnt_freeres(parent->sb->clntp, (xdrproc_t)xdr_diropres3, (char*)&res);
out2:
	DEBUG_OUT("the hsi_nfs3_symlink return with errno %d\n", err);
	return err;
}

