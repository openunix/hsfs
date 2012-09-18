#include "hsi_nfs3.h"

int hsi_nfs3_statfs (struct hsfs_inode *inode)
{ 
	struct hsfs_super *sb = inode->sb;
	struct fsstat3res res;	
	struct fsstat3resok resok;       
	struct nfs_fh3 fh;
	int st = 0;

	memset (&res, 0, sizeof(res));
	memset (&fh, 0, sizeof(fh));
	memset (&resok, 0, sizeof(resok));
	fh.data.data_len=inode->fh.data.data_len;
	fh.data.data_val=inode->fh.data.data_val;

	DEBUG_IN (" fh:%s",fh.data.data_val);

	st = hsi_nfs3_clnt_call (sb, sb->clntp, NFSPROC3_FSSTAT,
			(xdrproc_t)xdr_nfs_fh3, (caddr_t)&fh, 
			(xdrproc_t)xdr_fsstat3res, (caddr_t)&res);
	if (st)	
		goto out;

	st = res.status;
	if (NFS3_OK != st){

		st = hsi_nfs3_stat_to_errno (st);
		ERR ("rpc request failed: %d\n",st);
		clnt_freeres (sb->clntp, (xdrproc_t)xdr_fsstat3res,(char *)&res);
		goto out;
	}
	resok = res.fsstat3res_u.resok;
	inode->sb->tbytes = resok.tbytes;
	inode->sb->fbytes = resok.fbytes;
	inode->sb->abytes = resok.abytes;
	inode->sb->tfiles = resok.tfiles;
	inode->sb->ffiles = resok.ffiles;
	inode->sb->afiles = resok.afiles;
	clnt_freeres (sb->clntp, (xdrproc_t)xdr_fsstat3res,(char *)&res);
out:	
	DEBUG_OUT (" err:%d",st);
	return st;

}


#ifdef HSFS_NFS3_TEST
#include <sys/errno.h>
int main (int argc, char *argv[])
{
	int err;
	nfs_fh3 fh;
	size_t length;
	char *fh_v;
	char *svraddr = NULL;
	CLIENT *p_clntp = NULL;
	char *fpath =NULL;
	struct hsfs_inode root;    
	struct hsfs_super super;
	struct statvfs stbuf;
	root.sb = &super;
	char *cliname = basename (argv[0]);

	if (argc <3){
		err = EINVAL;
		fprintf(stderr, "%s $svraddr $fpath.\n", cliname);
		goto exit;
	}
	svraddr = argv[1];
	fpath = argv[2];
	p_clntp = clnt_create(svraddr, NFS_PROGRAM, NFS_V3,"udp");
	if (NULL == p_clntp){
		fprintf(stderr, "%s: Create handle to RPC server"\
				"(%s, %u, %u) failed: (%s).\n",cliname,
				svraddr, NFSPROC3_FSSTAT, NFS_V3,clnt_spcreateerror(cliname));
		err = ENXIO;
		goto exit;

	}
	err = map_path_to_nfs3fh (svraddr, fpath, &length, &fh_v);
	if (err!= 0){
		printf("map_path_to_nfs3fh failed! err:%d\n",err);
		goto exit;
	}
	memset (&fh,0,sizeof(fh));
	fh.data.data_len=length;
	fh.data.data_val=fh_v;
	root.fh = fh;

	super.clntp = p_clntp;
	err = hsi_nfs3_statfs(&root);
	if (err){
		printf("hsi_nfs3_statfs failed! err:%d\n",err);
		goto exit;
	}
	printf ("\ttbytes=%u\n\tfbytes=%u\n\tabytes=%u\n\ttfiles=%u\n\tffiles=%u\n\tafiles=%u\n",
			root.sb->tbytes,root.sb->fbytes,root.sb->abytes,root.sb->tfiles,root.sb->ffiles,root.sb->afiles);

	err = hsi_super2statvfs (root.sb, &stbuf);
	if (err){
		goto exit;
	}
	printf ("\tf_bsize=%u\n\tf_frsize=%u\n\tf_blocks=%u\n\tf_bfree=%u\n\tf_bavail=%u\n\tf_files=%u\n",
			stbuf.f_bsize,stbuf.f_frsize,stbuf.f_blocks,stbuf.f_bfree,stbuf.f_bavail,stbuf.f_files);
	printf ("\tf_ffree=%u\n\tf_favail=%u\n\tf_fsid=%u\n\tf_flag=%u\n\tf_namemax=%u\n",
			stbuf.f_ffree,stbuf.f_favail,stbuf.f_fsid,stbuf.f_flag,stbuf.f_namemax);

exit:
	exit (err);
}
#endif
