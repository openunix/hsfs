#include "hsi_nfs3.h"
#include "hsx_fuse.h"
#include "nfs3.h"
#include "acl.h"

#define NFS3_DIR 2
int hsi_nfs3_setxattr(struct hsfs_inode *inode, const char *value, int type,
			size_t size)
{
	SETACL3args args;	/*the struct of clnt_cal need data*/
	SETACL3res clnt_res;    /*the data of clnt_call result*/
	struct posix_acl *acl = NULL;
	struct posix_acl *alloc = NULL;
	struct posix_acl  *dfacl = NULL;
	int error = 0;
	int mask = 0;
	int i = 0;

	DEBUG_IN("in ino: %lu, mask %d\n", inode->ino, mask);

	if (type == ACL_TYPE_ACCESS)
		mask |= NA_ACLCNT | NA_ACL;	
		DEBUG("in hsi_nfs3_setxattr mask:%u,NFS_ACL %u..........", mask,
			NFS_ACL);

	if(inode->attr.type == 2) {
		mask |= NA_DFACLCNT | NA_DFACL;
		DEBUG("in hsi_nfs3_setxattr mask:%u,NFS_ACL %u..........", mask,
			NFS_ACL);
	}       

	memset (&args, 0, sizeof(SETACL3args));
	memset (&clnt_res, 0, sizeof(SETACL3res));
	
	acl = posix_acl_from_xattr(value, size);
	args.fh.data.data_len = inode->fh.data.data_len;
	args.fh.data.data_val = inode->fh.data.data_val;
	args.acl.mask = NFS_ACL;

	if(inode->attr.type == NFS3_DIR) {
		args.acl.mask |= NFS_DFACL;
		switch(type) {
			case ACL_TYPE_ACCESS:
				DEBUG("ACL_TYPE_ACCESS.......");
				if (hsi_nfs3_getxattr(inode, mask, &dfacl,
					type)) {
					DEBUG("here i am. \n");
					goto fail;
				}
				alloc = dfacl;
				break;

			case ACL_TYPE_DEFAULT:
				DEBUG("ACL_TYPE_DEACCESS.......");
				dfacl = acl;
				if (hsi_nfs3_getxattr(inode, mask, &acl, type))
					goto fail;
				alloc = acl;
				break;

			default:
				goto fail; 
		}
	} else if (type != ACL_TYPE_ACCESS)
			goto fail;;
	
	if (acl == NULL) {
		alloc = acl = posix_acl_from_mode(inode->attr.mode);
		if (!alloc) {
			DEBUG("error in acl from mode\n");
			goto fail;
		}
	}

	args.acl.aclcnt = acl->a_count;
	args.acl.aclent.aclent_len = acl->a_count;
	//args.acl.aclent.aclent_val = (aclent *)acl->a_entries;
	args.acl.aclent.aclent_val = (aclent *) calloc(acl->a_count,
					sizeof(struct aclent));
	
	for (i = 0; i < acl->a_count; i++) {
		args.acl.aclent.aclent_val[i].type = acl->a_entries[i].e_tag;
		args.acl.aclent.aclent_val[i].id = acl->a_entries[i].e_id;
		args.acl.aclent.aclent_val[i].perm = acl->a_entries[i].e_perm;
	}

	DEBUG("getxattr is ok. ........acl.type = %d, acl.id = %d, acl.perm ="
		"%d\n", args.acl.aclent.aclent_val[0].type,
			args.acl.aclent.aclent_val[0].id,
			args.acl.aclent.aclent_val[0].perm);	
	if (dfacl) {
		args.acl.dfaclcnt = dfacl->a_count;
		args.acl.dfaclent.dfaclent_len = dfacl->a_count;	
		args.acl.dfaclent.dfaclent_val = (aclent *) calloc (
						dfacl->a_count, 
						sizeof(struct aclent));		

		for (i = 0; i < dfacl->a_count; i++) {
			args.acl.dfaclent.dfaclent_val[i].type = dfacl->a_entries[i].e_tag;
			args.acl.dfaclent.dfaclent_val[i].id = dfacl->a_entries[i].e_id;
			args.acl.dfaclent.dfaclent_val[i].perm = dfacl->a_entries[i].e_perm;
		}

		//args.acl.dfaclent.dfaclent_val = (aclent *)dfacl->a_entries;
	}
	
	error = hsi_nfs3_clnt_call(inode->sb, inode->sb->acl_clntp, ACLPROC3_SETACL,
				(xdrproc_t) xdr_SETACL3args, (caddr_t) &args,
				(xdrproc_t) xdr_SETACL3res,(caddr_t) &clnt_res);
	
	DEBUG("error number in hsi_nfs3_clnt_call %d\n", error);
	if(error) {
		goto fail;
	}  else
		error = hsi_nfs3_stat_to_errno(clnt_res.status);
	clnt_freeres(inode->sb->acl_clntp, (xdrproc_t)xdr_SETACL3res,
			 (char *)&clnt_res);
fail:
	free(args.acl.aclent.aclent_val);
	free(args.acl.dfaclent.dfaclent_val);
	DEBUG("....................");
	free(acl);
	DEBUG("....................");
	free(alloc);
	DEBUG("....................");
	return error;
}
