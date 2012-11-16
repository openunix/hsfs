
#include <stdio.h>

#include "hsi_nfs3.h"
#include "log.h"

static inline void hsi_fsinfo_to_super(struct hsfs_super *super, 
					struct fsinfo3resok *fsinfo)
{
	if (super->rsize == 0)
		super->rsize = hsfs_block_size(fsinfo->rtpref, NULL);
	if (super->wsize == 0)
		super->wsize = hsfs_block_size(fsinfo->wtpref, NULL);

	if (fsinfo->rtmax >= 512 && super->rsize > fsinfo->rtmax)
		super->rsize = hsfs_block_size(fsinfo->rtmax, NULL);
	if (fsinfo->wtmax >= 512 && super->wsize > fsinfo->wtmax)
		super->wsize = hsfs_block_size(fsinfo->wtmax, NULL);

	if (super->rsize > HSFS_MAX_FILE_IO_SIZE)
		super->rsize = HSFS_MAX_FILE_IO_SIZE;

	if (super->wsize > HSFS_MAX_FILE_IO_SIZE)
		super->wsize = HSFS_MAX_FILE_IO_SIZE;

	super->dtsize = hsfs_block_size(fsinfo->dtpref, NULL);
	if (super->dtsize > HSFS_PAGE_SIZE * HSFS_MAX_READDIR_PAGES)
		super->dtsize = HSFS_PAGE_SIZE * HSFS_MAX_READDIR_PAGES;

	if (super->dtsize > super->rsize)
		super->dtsize = super->rsize;

	super->rtmax = fsinfo->rtmax;
	super->rtpref = fsinfo->rtpref;
	super->rtmult = fsinfo->rtmult;
	super->wtmax = fsinfo->wtmax;
	super->wtpref = fsinfo->wtpref;
	super->wtmult = hsfs_block_bits(fsinfo->wtmult, NULL);
	super->dtpref = fsinfo->dtpref;
	super->maxfilesize = fsinfo->maxfilesize;
	super->time_delta = fsinfo->time_delta;
	super->properties = fsinfo->properties;
}

int hsi_nfs3_fsinfo(struct hsfs_super *sb, nfs_fh3 *fh, struct nfs_fattr *fattr)
{
	CLIENT *clnt = sb->clntp;
	fsinfo3res res;
	int ret = 0;

	DEBUG_IN("(%p, %p, %p)", sb, fh, fattr);
       
	memset(&res, 0, sizeof(res));
	ret = hsi_nfs3_clnt_call(sb, clnt, NFSPROC3_FSINFO,
			(xdrproc_t)xdr_nfs_fh3, (char *)fh,
			(xdrproc_t)xdr_fsinfo3res, (char *)&res);

	if (ret)
		goto out;

	if (res.status != 0) {
		ERR("Got error when calling FSINFO: %d.", res.status);
		ret = hsi_nfs3_stat_to_errno(res.status);
		goto fres;
	}

	hsi_fsinfo_to_super(sb, &res.fsinfo3res_u.resok);
	hsi_nfs3_post2fattr(&res.fsinfo3res_u.resok.obj_attributes, fattr);
	if (!(fattr->valid & NFS_ATTR_FATTR))
		ret = hsi_nfs3_do_getattr(sb, fh, fattr, NULL);

fres:
	clnt_freeres(clnt, (xdrproc_t)xdr_fsinfo3res, (char *)&res);
out:
	DEBUG_OUT("(%d)", ret);

	return ret;
}
