/*hsi_nfs3_read.c*/


#include "hsi_nfs3.h"

int hsi_nfs3_read(struct hsfs_rw_info* rinfo)
{
	struct read3args args;
	struct read3res res;
	struct read3resok * resok = NULL;
	enum nfsstat3 st;
	struct timeval to = {120, 0};

	memset(&args, 0, sizeof(args));
	memset(&res, 0, sizeof(res));
	args.file.data.data_len = rinfo->inode->fh.data.data_len;
	args.file.data.data_val = rinfo->inode->fh.data.data_val;
	args.offset = rinfo->rw_off;
	args.count = rinfo->rw_size;
	//to.seconds = rinfo->inode->sb->timeo;
	//to.nseconds = 0;

	st = clnt_call(rinfo->inode->sb->clntp, NFSPROC3_READ, (xdrproc_t)xdr_read3args, (char *)&args, 
		(xdrproc_t)xdr_read3res, (char *)&res, to);
	if (st) {
		fprintf(stderr, "Call RPC Server failure: "\
			"(%s).\n",\
			clnt_sperrno(st));
		goto out;
	}

	if(NFS3_OK != res.status)
	{
		fprintf(stderr, "hsi_nfs3_read failure: %x ", res.status);
		st = res.status;
		goto out;
	}	

	resok = &res.read3res_u.resok;
	fprintf(stderr, "hsi_nfs3_read OUTPUT:\n"\
			"	read3resok.count:%x\n	read3resok.eof:%x\n",\
			resok->count, resok->eof);
	
	rinfo->data.data_val = resok->data.data_val;
	rinfo->data.data_len = resok->data.data_len;
	rinfo->ret_count = resok->count;
	rinfo->eof = resok->eof;

out:
	return st;
}

