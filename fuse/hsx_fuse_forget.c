#include "hsfs.h"
#include "hsx_fuse.h"

void hsx_fuse_forget(fuse_req_t req, fuse_ino_t ino, unsigned long nlookup)
{
	struct hsfs_inode  *hsfs_node;
	struct hsfs_super  *sb;
	
	if(ino == FUSE_ROOT_ID)
		goto out;

	sb = fuse_req_userdata(req);	
	hsfs_node = hsx_fuse_iget(sb, ino);
	if(hsfs_node == NULL)
		goto out ;
	
	while(hsfs_node->nlookup >= nlookup)
		hsfs_node->nlookup -= nlookup;
	if(! hsfs_node->nlookup)
	{
		hsx_fuse_idel(sb, hsfs_node);
	}
out:
	fuse_reply_none(req);

}
