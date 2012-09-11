/*
 *int hsi_nfs3_symlink(struct hsfs_inode *parent, struct hsfs_inode **new,
 *                    const char *nfs_link, const char *nfs_name)
 *xuehaitao
 *2012.9.6
 */
#include "hsfs.h"
#include "hsi_nfs3.h"
#include "hsx_fuse.h"
#include "log.h"

void hsx_fuse_symlink(fuse_req_t req, const char *link,
                      fuse_ino_t parent, const char *name)
{
        int st = 0;
        int err = 0;
	struct hsfs_super *sb_parent;
        struct hsfs_inode *nfs_parent;
        struct hsfs_inode *new = NULL;
	struct fuse_entry_param *e = NULL;

	DEBUG_IN("%s\n","hsx_fuse_symlink.");	

    	e = (struct fuse_entry_param *)malloc(sizeof(struct fuse_entry_param));

        sb_parent = fuse_req_userdata(req);
        nfs_parent = hsx_fuse_iget(sb_parent, parent);

        st = hsi_nfs3_symlink(nfs_parent, &new, link, name);
	if(st != 0){
        	err = st;
		goto out;
	}
	hsx_fuse_fill_reply(new, e);
	if(st != 0){
  		err = st;
  		goto out; 
	}
        st = fuse_reply_entry(req, e);
out:
	free(e);
	if(err != 0){
        	fuse_reply_err(req, err);
	}

	DEBUG("hsx_fuse_symlink failed.%d\n", err);

	return;
}
