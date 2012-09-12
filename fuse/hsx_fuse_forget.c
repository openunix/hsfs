#include "hsfs.h"
#include "hsx_fuse.h"

void hsx_fuse_forget(fuse_req_t req, fuse_ino_t ino, unsigned long nlookup)
{
	struct hsfs_inode  *hsfs_node;
	struct hsfs_super  *sb;
	
	DEBUG_IN("%s","");
	if(ino == FUSE_ROOT_ID)
		goto out;

	sb = fuse_req_userdata(req);	
	hsfs_node = hsx_fuse_iget(sb, ino);
	if(hsfs_node == NULL)
		goto out;
	
	if(hsfs_node->nlookup >= nlookup)
	{
		DEBUG("Now ,the node lookup count : %lu, the number of "
			"lookups to forget : %lu", hsfs_node->nlookup,nlookup);
		hsfs_node->nlookup -= nlookup;
	}
	else
		goto out;
	if(! hsfs_node->nlookup)
	{
		hsx_fuse_idel(sb, hsfs_node);
	}
out:
	DEBUG_OUT("%s","");
	fuse_reply_none(req);

}
