#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "hsi_nfs3.h"



int  hsi_nfs3_getxattr(struct hsfs_inode *hsfs_node, u_int mask, 
			struct posix_acl **pval , int type)
{
	struct GETACL3args args;
	struct GETACL3res  res;
	struct secattr *acl = NULL;
	size_t real_size = 0;

	enum clnt_stat	st;
	int i,err = 0;

	DEBUG_IN("%s","");
	memset(&args, 0, sizeof(args));
	memset(&res, 0, sizeof(res));

	args.fh = hsfs_node->fh;
	args.mask = mask;
	
	err = hsi_nfs3_clnt_call(hsfs_node->sb,hsfs_node->sb->acl_clntp,
		ACLPROC3_GETACL,(xdrproc_t)xdr_GETACL3args,(caddr_t)&args, 
		(xdrproc_t)xdr_GETACL3res, (caddr_t)&res);
	if(err)
	{
		ERR("Call acl rpc failed  ");
		goto out;
        }
	st = res.status;
        if (NFS3_OK != st)
	{
		ERR("Obtain extern attribute failure : (%d) !", st);
		err = hsi_nfs3_stat_to_errno(st);
		clnt_freeres(hsfs_node->sb->acl_clntp, (xdrproc_t)xdr_GETACL3res,
						(char *)&res);
		goto out;
        }
	acl = &res.GETACL3res_u.resok.acl;

	if(type == ACL_TYPE_ACCESS)
	{
		real_size = acl->aclent.aclent_len;
	}
	else if(type == ACL_TYPE_DEFAULT)
	{
		real_size = acl->dfaclent.dfaclent_len;
	}

	*pval = posix_acl_alloc(real_size);
	if(*pval == NULL)
	{
		err = ENOMEM ;
		ERR("Memory allocation failure !");
		clnt_freeres(hsfs_node->sb->clntp, (xdrproc_t)xdr_GETACL3res,
		                                (char *)&res);
		goto out;
	}
	if(type == ACL_TYPE_ACCESS)
	{
		DEBUG("%s","type == ACL_TYPE_ACCESS");
		for(i=0; i<(*pval)->a_count; i++)
		{
			(*pval)->a_entries[i].e_tag = acl->aclent.aclent_val[i].
							type;
			(*pval)->a_entries[i].e_perm = acl->aclent.aclent_val[i].
							perm;
			(*pval)->a_entries[i].e_id = acl->aclent.aclent_val[i].
							id;
		}
	}
	else if(type == ACL_TYPE_DEFAULT)
	{
		DEBUG("%s","type == ACL_TYPE_DEFAULT");
		for(i=0; i<(*pval)->a_count; i++)
		{
			(*pval)->a_entries[i].e_tag = acl->dfaclent.
							dfaclent_val[i].type;
			(*pval)->a_entries[i].e_perm = acl->dfaclent.
							dfaclent_val[i].perm;
			(*pval)->a_entries[i].e_id = acl->dfaclent.
							dfaclent_val[i].id;
		}
	}
	
	clnt_freeres(hsfs_node->sb->acl_clntp, (xdrproc_t)xdr_GETACL3res,
	                                (char *)&res);
out:
	DEBUG_OUT("%s","");
        return(err);
}


