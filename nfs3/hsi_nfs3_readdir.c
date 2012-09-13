
/*
 *hsi_nfs3_readdir.c
 */

#include <errno.h>
#include "hsfs.h"
#include "hsi_nfs3.h"
#include "log.h"

int hsi_nfs3_readdir(struct hsfs_inode *hi, struct hsfs_readdir_ctx *hrc, 
					size_t *dircount, size_t maxcount)
{
	CLIENT *clntp = NULL;
	struct hsfs_readdir_ctx *temp_hrc = NULL; 
	struct hsfs_readdir_ctx *temp_hrc1 = NULL;
	struct readdirplus3resok *resok = NULL;
	struct entryplus3 *temp_entry = NULL;
	struct readdirplus3args args;
	struct readdirplus3res res;
	struct timeval to;
	size_t dir_size = 0;
	int err = 0;

	DEBUG_IN("%s.","hsx_fuse_readdir");
	memset(&args, 0, sizeof(args));
	memset(&res, 0, sizeof(res));
	args.cookie = hrc->off;
	memcpy(args.cookieverf, hrc->cookieverf, NFS3_COOKIEVERFSIZE);
	args.dir.data.data_val = hi->fh.data.data_val;
	args.dir.data.data_len = hi->fh.data.data_len;
	args.maxcount = maxcount;
	dircount = (size_t *)&(args.dircount);
	temp_hrc1 = hrc;
	to.tv_sec = hi->sb->timeo / 10;
	to.tv_usec = (hi->sb->timeo % 10) * 100000;
	clntp = hi->sb->clntp;

	do{		
		err = clnt_call(clntp, NFSPROC3_READDIRPLUS,
				(xdrproc_t)xdr_readdirplus3args, &args,
				(xdrproc_t)xdr_readdirplus3res, &res, to);
		if (err) {	
			ERR("Call RPC Server failure:(%s).\n", 
							clnt_sperrno(err));
			err = hsi_rpc_stat_to_errno(hi->sb->clntp);
			goto out;
		}

		if (NFS3_OK != res.status) {
			ERR("Call NFS3 Server failure:(%d).\n", err);
			err = hsi_nfs3_stat_to_errno(res.status);
			clnt_freeres(clntp, (xdrproc_t)xdr_readdirplus3res,
                                                                (char *)&res);
			goto out;
		}
 
		resok = &res.readdirplus3res_u.resok;
		temp_entry = resok->reply.entries;
		while(temp_entry != NULL){
			temp_hrc = (struct hsfs_readdir_ctx *)malloc(sizeof
						(struct hsfs_readdir_ctx));
			if(temp_hrc == NULL){
				err = ENOMEM;
				ERR("Temp_hrc memory leak.");
				goto out;
			}

			memset(temp_hrc, 0, sizeof(struct hsfs_readdir_ctx));
			temp_hrc->name = (char *)malloc(strlen
						(temp_entry->name) + 1);
			if (temp_hrc->name == NULL) {
				err = ENOMEM;
				ERR("Temp_hrc->name memory leak.");
				goto out;
			}

			memcpy(temp_hrc->cookieverf, resok->cookieverf,
						 NFS3_COOKIEVERFSIZE);
			strcpy(temp_hrc->name, temp_entry->name);
			temp_hrc->off = temp_entry->cookie;
			err = resok->reply.eof;
			if (err == 0 && temp_entry->nextentry == NULL){
				args.cookie = temp_entry->cookie;
				memcpy(args.cookieverf, resok->cookieverf, 
						NFS3_COOKIEVERFSIZE);
			}
	 
			err = temp_entry->name_attributes.present;
			if (1 != err) {
				ERR("Rpc get fattr3 failure: %s.", 
							clnt_sperrno(err));
				goto out;
			}
	    		
			err = hsi_nfs3_fattr2stat(&temp_entry->
						name_attributes.post_op_attr_u.attributes,
						&temp_hrc->stbuf);
			
			if(err != 0){
				ERR("Set fattr3 to stat failure.");
				goto out;
	
			}
			temp_hrc1->next = temp_hrc;
			temp_hrc1 = temp_hrc;
			temp_entry = temp_entry->nextentry;
		}
	  
		dir_size += *dircount;
		if (dir_size > maxcount)
			break;

		clnt_freeres(clntp, (xdrproc_t)xdr_readdirplus3res, 
								(char *)&res);

		err = resok->reply.eof;
	}while(err == 0);
 
	err = 0;

out:
	DEBUG_OUT("%s.","hsx_fuse_readdir");
	return err;
}

#ifdef HSFS_NFS3_TEXT
int main(int argc, char *argv[])
{
	char *svraddr = NULL;
	CLIENT *clntp = NULL;
	char *fpath = NULL;
	size_t fhLen = 0;
    	unsigned char *fhvalp = NULL;
	struct hysfattr3 *fattrp = NULL;
	struct hsfs_inode *hi = NULL;
	struct hsfs_readdir_ctx *hrc = NULL;
	struct rpc_err rerr;
	size_t *dircount = NULL;
	size_t maxcount = 0;
	struct timeval to = {10, 0};
	enum clnt_stat st;
	int err = 0;

	svraddr = argv[1];
	fpath = argv[2];
	
	err = map_path_to_nfs3fh(svraddr, fpath, &fhLen, &fhvalp);
	if(err)
	{
	  printf("Map path to nfs3fh failed.");
	  goto out;
	}
	
	clntp = clnt_create(svraddr, NFS_PROGRAM, NFS_V3, "tcp");
	if (NULL == clntp) {
		err = 3;
		printf("Create client failed.");
		goto out;
	} 
	
	hi = (struct hsfs_inode*)malloc(sizeof(struct hsfs_inode));
	hrc = (struct hsfs_readdir_ctx*)malloc(sizeof(struct hsfs_readdir_ctx));
	memset(hrc, 0, sizeof(struct hsfs_readdir_ctx));
	hi->sb = (struct hsfs_super*)malloc(sizeof(struct hsfs_super));
	hi->sb->clntp = clntp;
	hi->fh.data.data_val = fhvalp ;
	hi->fh.data.data_len = fhLen;
	maxcount = 8192;
	err = hsi_nfs3_readdir(hi, hrc, dircount, maxcount);
	if (err) {
		ERR("Call RPC Server failure:%s", clnt_sperrno(err));
                clnt_geterr(hi->sb->clntp, &rerr);
                err = rerr.re_errno;
		goto out;
	}
	
	while(hrc->next != NULL){	
		printf("the cookieverf is 0x%llx.\n", 
			*(long long *)hrc->next->cookieverf);
		printf("the name of entry is %s, the offset is %d.\n", 
			hrc->next->name, hrc->next->off);
		hrc = hrc->next;
	}

out:
	if (NULL != clntp)
		clnt_destroy(clntp);
	exit(err);
}
#endif /* HSFS_NFS3_TEXT */
