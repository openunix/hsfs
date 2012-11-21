
#include "hsi_nfs3.h"
#include "log.h"

int hsi_nfs3_pathconf(struct hsfs_inode *inode)
{
	struct hsfs_super *sb = inode->sb;
	CLIENT *clnt = sb->clntp;
	pathconf3res res;
	int ret = 0;
	struct nfs_fh3 fh;

	DEBUG_IN("ino: %lu.", inode->ino);

	memset(&res, 0, sizeof(res));

	hsi_nfs3_getfh3(inode, &fh);
	ret = hsi_nfs3_clnt_call(sb, clnt, NFSPROC3_PATHCONF,
			(xdrproc_t)xdr_nfs_fh3, (char *)&fh,
			(xdrproc_t)xdr_pathconf3res, (char *)&res);

	if (ret)
		goto out;

	if (res.status != 0) {
		ERR("Got error when calling PATHCONF: %d.", res.status);
		ret = hsi_nfs3_stat_to_errno(res.status);
		goto fres;
	}

	sb->namlen = res.pathconf3res_u.resok.name_max;

	/* We don't need to update inode here */

/* 	pattr = &res.pathconf3res_u.resok.obj_attributes; */
/* 	if (pattr->present) { */
/* /\* 		memcpy(&sb->root->attr, &pattr->post_op_attr_u.attributes, *\/ */
/* /\* 			sizeof(fattr3)); *\/ */
/* 	} */
fres:
	clnt_freeres(clnt, (xdrproc_t)xdr_pathconf3res, (char *)&res);
out:
	DEBUG_OUT("(%d, %d)", ret, sb->namlen);

	return ret;
}
