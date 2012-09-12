
#include "hsi_nfs3.h"
#include "log.h"

int hsi_nfs3_pathconf(struct hsfs_inode *inode)
{
	struct hsfs_super *sb = inode->sb;
	CLIENT *clnt = sb->clntp;
	pathconf3res res = {};
	post_op_attr *pattr = NULL;
	struct timeval to = {sb->timeo / 10, sb->timeo % 10 * 100};
	enum clnt_stat st;
	int ret = 0;

	DEBUG_IN("ino: %lu.", inode->ino);

	st = clnt_call(clnt, NFSPROC3_PATHCONF, (xdrproc_t)xdr_nfs_fh3,
			 (char *)&inode->fh, (xdrproc_t)xdr_pathconf3res,
			 (char *)&res, to);

	if (st != RPC_SUCCESS) {
		ERR("Sending rpc request failed: %d.", st);
		ret = hsi_rpc_stat_to_errno(clnt);
		goto out;
	}

	if (res.status != 0) {
		ERR("Got error when calling PATHCONF: %d.", res.status);
		ret = hsi_nfs3_stat_to_errno(res.status);
		goto fres;
	}

	sb->namlen = res.pathconf3res_u.resok.name_max;

	pattr = &res.pathconf3res_u.resok.obj_attributes;
	if (pattr->present) {
		memcpy(&sb->root->attr, &pattr->post_op_attr_u.attributes,
			sizeof(fattr3));
	}
fres:
	clnt_freeres(clnt, (xdrproc_t)xdr_pathconf3res, (char *)&res);
out:
	DEBUG_OUT("%s", ".");

	return ret;
}
