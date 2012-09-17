#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "hsi_nfs3.h"

#ifdef HSFS_NFS3_TEST
#include "apis.h"
struct hsfs_super s;
struct hsfs_super *sb=&s;
#endif /* HSFS_NFS3_TEST */

int hsi_nfs3_lookup(struct hsfs_inode *parent,struct hsfs_inode **newinode, 
		const char *name)
{
	struct hsfs_super *sb = parent->sb;
	struct diropargs3 args;
	struct lookup3res res;
	struct fattr3 *pattr = NULL;
	nfs_fh3	*name_fh;
	int st = 0, err = 0;

	memset(&args, 0, sizeof(args));
	memset(&res, 0, sizeof(res));

	DEBUG_IN("%s","");
	args.dir = parent->fh;
	args.name = (char *)name;
	err=hsi_nfs3_clnt_call(sb, sb->clntp, NFSPROC3_LOOKUP,
			(xdrproc_t)xdr_diropargs3,(caddr_t)&args,
			(xdrproc_t)xdr_lookup3res, (caddr_t)&res);

	if (err) 
		goto out;

	st = res.status;
	if (NFS3_OK != st) 
	{
		ERR("Path (%s) on Server is not "
			"accessible: (%d).",name,st);
		err = hsi_nfs3_stat_to_errno(st);
		clnt_freeres(sb->clntp, (xdrproc_t)xdr_lookup3res, 
			(char *)&res);
		goto out;
	}
	if (! res.lookup3res_u.resok.obj_attributes.present)
	{
		ERR("Acquired property is invalid !");
		err = EINVAL;
		clnt_freeres(sb->clntp, (xdrproc_t)xdr_lookup3res, (char *)&res);
		goto out;
	}
	
	pattr=&res.lookup3res_u.resok.obj_attributes.post_op_attr_u.attributes;
	name_fh = &res.lookup3res_u.resok.object;
        
	*newinode = hsi_nfs3_ifind(parent->sb,name_fh,pattr);
	clnt_freeres(sb->clntp, (xdrproc_t)xdr_lookup3res, (char *)&res);

out:
	DEBUG_OUT(" with errno %d",err);
	return(err);

}

#ifdef HSFS_NFS3_TEST

int main(int argc ,char *argv[])
{
	char *svraddr = NULL;
	char *cliname = NULL;
	CLIENT *clntp = NULL;
	char *fpath = NULL;
	size_t fhlen = 0;
	unsigned char *pnfsfh = NULL;
	
	struct hsfs_inode parent;
        struct hsfs_inode *child;

	int err = 0;

	cliname = basename(argv[0]);

	if (argc < 4)
	{
		err = EINVAL;
		fprintf(stderr, "%s $svraddr $fpath.\n", cliname);
		goto out;
	}
	hsx_fuse_itable_init(sb);
	svraddr = argv[1];
	fpath = argv[2];

	clntp = clnt_create(svraddr, NFS_PROGRAM, NFS_V3, "udp");
	if (NULL == clntp) 
	{
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

	if((err = hsi_nfs3_lookup(&parent,&child,argv[3])))
	{
		fprintf(stderr,"Call hsi_nfs3_lookup function fails,Error"
			"code : %d\n",err);	
		goto out;
	}

	fprintf(stdout, "Attributes of path '%s':\n""\tino=%lu\t\n\t"\
                        "generation=%lu\t\n\t""nlookup=%lu\t\n\t"\
                        "type=%u\t\n\t""mode=%08x\t\n\t"\
                        "nlink=%u\n\tuid=%u\n\tgid=%u\n\t"\
                        "size=%lu\n\tused=%lu\n\t"\
                        "fsid=%lu\n\tfileid=%lu\n",argv[3],child->ino,
                        child->generation,child->nlookup,child->attr.type,
                        child->attr.mode,child->attr.nlink,child->attr.uid,
                        child->attr.gid,child->attr.size,child->attr.used,
                        child->attr.fsid,child->attr.fileid);

out:
	if (NULL != clntp)
		clnt_destroy(clntp);
	exit(err);
}

#endif/* HSFS_NFS3_TEST */


