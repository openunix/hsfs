/*
 *int hsi_nfs3_symlink(struct hsfs_inode *parent, struct hsfs_inode **new,
 *                    const char *nfs_link, const char *nfs_name)
 *xuehaitao
 *2012.9.6
 */


void hsx_fuse_symlink(fuse_req_t req, const char *link,
                      fuse_ino_t parent, const char *name)
{
        int st = 0;
        int err = 0;
	struct hsfs_super sb_parent;
        struct hsfs_super *sb_new = NULL;
        struct hsfs_inode parent;
        struct hsfs_inode *new = NULL;
	struct fuse_entry_param *e = NULL;
	
    	e = (struct fuse_entry_param *)malloc(sizeof(struct fuse_entry_param));
	new = (struct hsfs_inode*)malloc(sizeof(struct hsfs_inode));
        sb_new = (struct hsfs_super*)malloc(sizeof(struct hsfs_super));
        memset(&sb_parent, 0, sizeof(struct hsfs_super));
        new->sb = sb_new;
        parent.sb = &sb_parent;

        sb_parent = &fuse_req_userdata(req);
        parent = &hsx_fuse_iget(&sb_parent, ino);

        st = hsi_nfs3_symlink(&parent, &new, link, name);
	if(st != 0){
        	err = st;
		goto out;
	}
	st = hsx_fuse_fill_reply(new, &e);
	if(st != 0){
  		err = st;
  		goto out; 
	}
        st = fuse_reply_entry(req, e);
out:
   	free(new);
	free(e);
	free(sb_new);
	if(err != 0){
        	fuse_reply_err(req, err);
	}
	return 0;
}
