/*
 * hsx_fuse_readlink
 * xuehaitao
 * 2012.9.6
 */


void hsx_fuse_readlink(fuse_req_t req, fuse_ino_t ino){

        int st = 0;
				int err = 0;
        struct hsfs_inode hi;
        struct hsfs_super hi_sb;
        const char *link = NULL;
	
        memset(&hi, 0, sizeof(struct hsfs_inode));
        memset(&hi_sb, 0, sizeof(struct hsfs_super));
        hi->sb = &hi_sb;

        hi->sb = fuse_req_userdata(req);
        hi = hsx_fuse_iget(hi->sb, ino);
        
        st = hsi_nfs3_readlink(&hi, link);
        if(st != 0){
		            err = st;
                goto out;
	}
 	
        st = fuse_reply_readlink(req, link);
        if(st != 0){
		            err = st;
		            goto out;
	}

out:
        if(st != 0){
		            fuse_reply_err(req, err);
	}
        return 0;
}



