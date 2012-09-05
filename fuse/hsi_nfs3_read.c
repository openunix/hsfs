/*hsi_nfs3_read.c*/

#include "hsfs.h"
#include "hsi_nfs3.h"
#include "log.h"

int hsi_nfs3_read(struct hsfs_rw_info* rinfo)
{
	struct read3args args;
	struct read3res res;
	struct read3resok * resok = NULL;
	struct timeval to = {0, 0};
	struct rpc_err rerr;
	int err = 0;

	memset(&args, 0, sizeof(args));
	memset(&res, 0, sizeof(res));
	args.file.data.data_len = rinfo->inode->fh.data.data_len;
	args.file.data.data_val = rinfo->inode->fh.data.data_val;
	args.offset = rinfo->rw_off;
	args.count = rinfo->rw_size;
	to.tv_sec = rinfo->inode->sb->timeo / 10;
	to.tv_usec = (rinfo->inode->sb->timeo % 10) * 100;

	err = clnt_call(rinfo->inode->sb->clntp, NFSPROC3_READ,
		       (xdrproc_t)xdr_read3args, (char *)&args,
		(xdrproc_t)xdr_read3res, (char *)&res, to);
	if (err) {
		ERR("Call RPC Server failure:%s", clnt_sperrno(err));
		clnt_geterr(rinfo->inode->sb->clntp, &rerr);
		err = rerr.re_errno;
		goto out;
	}

	err = 5;//hsi_nfs3_stat_to_errno(res.status);
	if(err){
		ERR("hsi_nfs3_write failure: %x", err);
		goto out;
	}	

	resok = &res.read3res_u.resok;
	DEBUG("hsi_nfs3_read OUTPUT: read3resok.count:%x    read3resok.eof:%x",
			resok->count, resok->eof);
	rinfo->data.data_val = resok->data.data_val;
	rinfo->data.data_len = resok->data.data_len;
	rinfo->ret_count = resok->count;
	rinfo->eof = resok->eof;

out:
	return err;
}

