
#include "hsx_fuse.h"
#include "hsi_nfs3.h"

void hsx_fuse_init(void *data, struct fuse_conn_info *conn)
{
	struct hsfs_super *sb = (struct hsfs_super *)data;

	hsi_nfs3_fsinfo(sb->root);
	hsi_nfs3_pathconf(sb->root);
}

