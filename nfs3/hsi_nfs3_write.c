/*hsi_nfs3_write.c*/

#include <errno.h>
#include "hsi_nfs3.h"
#include "log.h"


int hsi_nfs3_write(struct hsfs_rw_info* winfo)
{
	struct write3args args;
	struct write3res res;
	struct write3resok * resok = NULL;
	struct timeval to = {0, 0};
	struct rpc_err rerr;
	int err = 0;

	memset(&args, 0, sizeof(args));
	memset(&res, 0, sizeof(res));
	args.file.data.data_len = winfo->inode->fh.data.data_len;
	args.file.data.data_val = winfo->inode->fh.data.data_val;
	args.data.data_len = winfo->data.data_len;
	args.data.data_val = winfo->data.data_val;
	args.offset = winfo->rw_off;
	args.count = winfo->rw_size;
	args.stable = winfo->stable;
	to.tv_sec = winfo->inode->sb->timeo / 10;
	to.tv_usec = (winfo->inode->sb->timeo % 10) * 100;

	err = clnt_call(winfo->inode->sb->clntp, NFSPROC3_WRITE,
		       (xdrproc_t)xdr_write3args, (char *)&args, 
		(xdrproc_t)xdr_write3res, (char *)&res, to);
	if(err){
		ERR("Call RPC Server failure: %s", clnt_sperrno(err));
		clnt_geterr(winfo->inode->sb->clntp, &rerr);
		err = rerr.re_errno == 0 ? EIO : rerr.re_errno ;
		goto out;
	}

	err = 0;//hsi_nfs3_stat_to_errno(res.status);
	if(err){
		ERR("hsi_nfs3_write failure: %x", err);
		goto out;
	}	

	resok = &res.write3res_u.resok;
	DEBUG("hsi_nfs3_write OUTPUT: count: %x", resok->count);
	winfo->ret_count = resok->count;

out:
	return err;
}

#ifdef HSFS_NFS3_TEST

int main(int argc, char *argv[])
{
	struct hsfs_rw_info winfo;
	struct hsfs_inode inode;
	struct hsfs_super sblock;
	char *svraddr = NULL;
	char *fpath = NULL;
	char *mpoint = NULL;
	char *buffer = NULL;
	CLIENT *clntp = NULL;
	size_t fhlen = 0 ;
	unsigned char *fhvalp = NULL;
	size_t iosize = 0;
	size_t fsize = 0;
	int err = 0;
	size_t i = 0;
	
	if(argc != 6){
		ERR("USEAGE: hsi_nfs3_write  SERVER_IP   FILEPATH   IOSIZE  FILESIZE"
			"MOUNTPOINT\n");
		ERR("EXAMPLE: ./hsi_nfs3_write	10.10.99.120	"
			"/nfsXport/a/file   512   1000000  /mnt/hsfs\n");
		err = EINVAL;
		goto out;
	}
	
	svraddr = argv[1];
	fpath = argv[2];
	iosize = atoi(argv[3]);
	fsize = atoi(argv[4]);
	if(0 == iosize | 0 == fsize)
		DEBUG("IOSIZE or FILESIZE is zero.\n");
	mpoint = argv[5];
	clntp = clnt_create(svraddr, NFS_PROGRAM, NFS_V3, "TCP");
	if(NULL == clntp){
		ERR("Create handle to RPC server (%s, %u, %u) failed\n",
			svraddr, NFS_PROGRAM, NFS_V3);
		err = EREMOTEIO;
		goto out;
	}

	err = map_path_to_nfs3fh(svraddr, fpath, &fhlen, &fhvalp);
	if(err){
		ERR("map_path_to_nfs3fh error: %s.\n", strerror(err));
		err = ENXIO;		
		goto out;
	}
	
	memset(&inode, 0, sizeof(struct hsfs_inode));
	memset(&winfo, 0, sizeof(struct hsfs_rw_info));
	memset(&sblock, 0, sizeof(struct hsfs_super));
	inode.sb = &sblock;
	winfo.inode = &inode;
	
	sblock.timeo = 1200;//120sec
	sblock.clntp = clntp;
	inode.fh.data.data_len = fhlen;
	inode.fh.data.data_val = fhvalp;
	
	winfo.rw_off = 0;
	winfo.rw_size = iosize;
	buffer = (char *) malloc(iosize);
	if( NULL == buffer){
		ERR("insufficient memory.\n");
		err = ENOMEM;
		goto out;
	}
	winfo.stable = HSFS_FILE_SYNC;
	memset(buffer, '0', iosize);
	winfo.data.data_len = iosize;
	winfo.data.data_val = buffer;
	for(i = 0; i < fsize; ){
		winfo.rw_off += winfo.ret_count;
		err = hsi_nfs3_write(&winfo);
		if(err)
			break;
		
		i += winfo.ret_count;
	}
out:
	if(NULL != buffer)
		free(buffer);

	return err;
}

#endif /*HSFS_NFS3_TEST*/