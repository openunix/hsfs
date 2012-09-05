/*hsi_nfs3_write.c*/

#include "hsfs.h"
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
		err = rerr.re_errno;
		goto out;
	}

	err = 5;//hsi_nfs3_stat_to_errno(res.status);
	if(err){
		ERR("hsi_nfs3_write failure: %x", err);
		goto out;
	}	

	resok = &res.write3res_u.resok;
	DEBUG("hsi_nfs3_write OUTPUT: write3resok.count: %x", resok->count);
	winfo->ret_count = resok->count;

out:
	return err;
}
