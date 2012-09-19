#include <errno.h>
#include "hsi_nfs3.h"
#include "hsx_fuse.h"
#include "acl.h"
void hsx_fuse_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name, size_t size)
{
	struct hsfs_inode *hsfs_node;
	struct hsfs_super *sb;
	int mask = 0;
	struct posix_acl *acl = NULL;
	char *buf = NULL;

	int  type,real_size = 0,err = 0;

	DEBUG_IN("The inode number : ino = %lu,name : %s",ino,name);
	if (strcmp(name, POSIX_ACL_XATTR_ACCESS) == 0)
		type = ACL_TYPE_ACCESS;
	else if (strcmp(name, POSIX_ACL_XATTR_DEFAULT) == 0)
		{
			type = ACL_TYPE_DEFAULT;
		}
	else
	{
		err = ENOTSUP;
		goto out;
	}
	sb = fuse_req_userdata(req);
	if((hsfs_node=hsx_fuse_iget(sb,ino)) == NULL)
	{
		err = ENOENT;
		goto out;
	}

	if (type == ACL_TYPE_ACCESS)
		mask |= NA_ACLCNT | NA_ACL;
	if(type == ACL_TYPE_DEFAULT)
	{
		mask |= NA_DFACLCNT | NA_DFACL;
	}
	
	if ((err=hsi_nfs3_getxattr(hsfs_node,mask,&acl,type)))
	{
		goto out ;
	}
		
	buf = (char *)calloc(1,size);
	if(buf == NULL && size)
	{
		err = ENOMEM;	
		free(acl);
		acl =NULL;
		goto out;	
	}

	real_size = posix_acl_to_xattr(acl, (void *)buf, size);
	if(size == 0)
	{
		free(acl);
		acl = NULL;
		fuse_reply_xattr(req,real_size);
		DEBUG_OUT("%s","success exit");
		return ;
	}
	if(real_size == ERANGE)
	{
		err = ERANGE;
		free(buf);
		free(acl);
		buf = NULL;
		acl = NULL;
		goto out;
	}
	fuse_reply_buf(req,buf,real_size);
	free(buf);
	free(acl);
	buf = NULL;
	acl = NULL;
	DEBUG_OUT("%s","success exit");
	return ;

out :
	fuse_reply_err(req,err);
	DEBUG_OUT("failed,with errno %d",err);
}
