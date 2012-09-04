/*hsi_nfs3_write.c*/


#include "hsfs.h"
#include "hsi_nfs3.h"

int hsi_nfs3_write(struct hsfs_rw_info* winfo)
{
	struct write3args args;
	struct write3res res;
	struct write3resok * resok = NULL;
	enum nfsstat3 st;
	struct timeval to = {120, 0};

	memset(&args, 0, sizeof(args));
	memset(&res, 0, sizeof(res));
	args.file.data.data_len = winfo->inode->fh.data.data_len;
	args.file.data.data_val = winfo->inode->fh.data.data_val;
	args.offset = winfo->rw_off;
	args.count = winfo->rw_size;
	args.stable = winfo->stable;
	//to.seconds = rinfo->inode->sb->timeo;
	//to.mseconds = 0;

	st = clnt_call(winfo->inode->sb->clntp, NFSPROC3_WRITE, (xdrproc_t)xdr_write3args, (char *)&args, 
		(xdrproc_t)xdr_write3res, (char *)&res, to);
	if (st) {
		fprintf(stderr, "Call RPC Server failure: "\
			"(%s).\n",\
			clnt_sperrno(st));
		goto out;
	}

	if(NFS3_OK != res.status)
	{
		fprintf(stderr, "hsi_nfs3_write failure: %x ", res.status);
		st = res.status;
		goto out;
	}	

	resok = &res.write3res_u.resok;
	fprintf(stderr, "hsi_nfs3_write OUTPUT:\n"\
			"	write3resok.count:%x\n",\
			resok->count);
	
	winfo->ret_count = resok->count;

out:
	return st;
}
