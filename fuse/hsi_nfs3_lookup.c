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

#include "../include/hsfs.h"
#include "../include/log.h"
#include "../include/hsi_nfs3.h"

#define TEST_LOOKUP

#ifdef TEST_LOOKUP
#include "apis.h"
struct hsfs_super s;
struct hsfs_super *sb=&s;
#endif

int  hsi_fuse_lookup(struct hsfs_inode *parent,struct hsfs_inode **newinode, char *name)
{
	struct diropargs3	args;
	struct lookup3res	res;
	struct fattr3_new	*pattr = NULL;
	nfs_fh3	name_fh;
	enum clnt_stat	st;
	int err = 0;
	struct timeval to={120,0};

	memset(&args, 0, sizeof(args));
	memset(&res, 0, sizeof(res));

	args.dir = parent->fh;
	args.name = name;
	st=clnt_call(parent->sb->clntp,3,(xdrproc_t)xdr_diropargs3,(caddr_t)&args,
			(xdrproc_t)xdr_lookup3res, (caddr_t)&res, to);

	if (st) {
		ERR("Call RPC Server (%u, %u) failure: "
			"(%s).\n",NFS_PROGRAM, NFS_V3, clnt_sperrno(st));
		err = hsi_rpc_stat_to_errno(parent->sb->clntp);
                goto out;
        }
        st = res.status;
        if (NFS3_OK != st) {
		ERR("Path (%s) on Server is not "
			"accessible: (%d).\n",name,st);
		err = hsi_nfs3_stat_to_errno(st);
		goto out;
        }
		
        pattr = &res.lookup3res_u.resok.obj_attributes.post_op_attr_new_u.attributes;
        name_fh = res.lookup3res_u.resok.object;
        
	*newinode = hsi_nfs3_ifind(sb,&name_fh,pattr);
	
out:
        return(err);

}

#ifdef	 TEST_LOOKUP

int main(int argc ,char *argv[])
{
	char *svraddr = NULL;
	char *cliname = NULL;
	CLIENT *clntp = NULL;
	char *fpath = NULL;
	size_t fhlen = 0;
	unsigned char *pnfsfh = NULL;
	nfs_fh3  parent_fh;
	
	struct hsfs_inode parent;
        struct hsfs_inode *child;


	enum clnt_stat st;
	int err = 0;

	cliname = basename(argv[0]);

	if (argc < 4) {
		err = EINVAL;
		fprintf(stderr, "%s $svraddr $fpath.\n", cliname);
		goto out;
	}
	hsx_fuse_init(sb);
	svraddr = argv[1];
	fpath = argv[2];

	clntp = clnt_create(svraddr, NFS_PROGRAM, NFS_V3, "udp");
	if (NULL == clntp) {
		fprintf(stderr, "%s: Create handle to RPC server "\
			"(%s, %u, %u) failed: (%s).\n", cliname,\
			svraddr, NFS_PROGRAM, NFS_V3, clnt_spcreateerror(cliname));
		err = ENXIO;
		goto out;
	} 
	
		
	if (map_path_to_nfs3fh(svraddr,fpath,&fhlen,&pnfsfh))
        {
                fprintf(stderr, "%s: Transfrom  %s  to FH failed  "\
                        "(%s, %u, %u) .\n", cliname,\
                        fpath,svraddr, NFS_PROGRAM, NFS_V3);
                goto out;
        }
	parent.sb = sb;
	parent.sb->clntp = clntp;
	parent.fh.data.data_val = (char *)pnfsfh;
	parent.fh.data.data_len = fhlen;

	hsi_fuse_lookup(&parent,&child,argv[3]);

	fprintf(stdout, "Attributes of path '%s':\n""\tino=%lu\t\n\t"\
                        "generation=%d\t\n\t""nlookup=%u\t\n\t"\
                        "type=%d\t\n\t""mode=%08x\t\n\t"\
                        "nlink=%u\n\tuid=%u\n\tgid=%u\n\t"\
                        "size=%u\n\tused=%u\n\t"\
                        "fsid=%u\n\tfileid=%lu\n",argv[3],child->ino,
                        child->generation,child->nlookup,child->attr.type,
                        child->attr.mode,child->attr.nlink,child->attr.uid,
                        child->attr.gid,child->attr.size,child->attr.used,
                        child->attr.fsid,child->attr.fileid);

out:
	if (NULL != clntp)
		clnt_destroy(clntp);
	exit(err);
}

#endif


