
/*
 *hsi_nfs3_readdir.c
 */

#include "hsfs.h"
#include "hsi_nfs3.h"
#include "log.h"

int hsi_nfs3_readdir(struct hsfs_inode *hi, struct hsfs_readdir_ctx *hrc, 
					size_t *dircount, size_t maxcount)
{
	CLIENT *clntp = NULL;
	struct hsfs_readdir_ctx *temp = NULL; 
	struct hsfs_readdir_ctx *temp1 = NULL;
	struct readdirplus3resok *resok = NULL;
	struct readdirplus3args args;
	struct readdirplus3res res;
	struct rpc_err rerr;
	struct timeval to = {10, 0};
	size_t dir_size = 0;
	int err = 0;
	int flag = 0;

	temp1 = hrc;
	memset(&args, 0, sizeof(args));
	memset(&res, 0, sizeof(res));
	args.cookie = hrc->off;
	memcpy(args.cookieverf, hrc->cookieverf, NFS3_COOKIEVERFSIZE);
	args.dir.data.data_val = hi->fh.data.data_val;
        args.dir.data.data_len = hi->fh.data.data_len;
	args.maxcount = maxcount;
	dircount = (size_t *)&args.dircount;
	args.dircount = 1024;
	clntp = hi->sb->clntp;

	do{
		
		err = clnt_call(clntp, NFSPROC3_READDIRPLUS,
				(xdrproc_t)xdr_readdirplus3args, &args,
				(xdrproc_t)xdr_readdirplus3res, &res, to);
		if (err) {	
			err = hsi_rpc_stat_to_errno(hi->sb->clntp);
			goto out;
		}

		if (NFS3_OK != res.status) {
			err = hsi_nfs3_stat_to_errno(res.status);
			goto out;
		}
 
		resok = &res.readdirplus3res_u.resok;
		while(resok->reply.entries != NULL){
			if (flag == 1){
				temp->off = resok->reply.entries->cookie;
				flag = 0; 
			}

		 	temp = (struct hsfs_readdir_ctx *)malloc(sizeof
				(struct hsfs_readdir_ctx));
			memset(temp, 0, sizeof(struct hsfs_readdir_ctx));
			temp->name = (char *)malloc(strlen
					(resok->reply.entries->name) + 1);
			if (temp->name == NULL) {
				ERR("Memory leak.");
				goto out;
			}

			memcpy(temp->cookieverf, resok->cookieverf,
						 NFS3_COOKIEVERFSIZE);
			strcpy(temp->name, resok->reply.entries->name);
			if (resok->reply.entries->nextentry != NULL)
			  	 temp->off = resok->reply.entries->nextentry
				->cookie;
			else{
				err = resok->reply.eof;
			  	if (err == 0){
					flag = 1;
			      		args.cookie = 
						resok->reply.entries->cookie;
			     		memcpy(args.cookieverf, 
							resok->cookieverf, 
							NFS3_COOKIEVERFSIZE);
				}
				else{
					temp->off =  
						resok->reply.entries->cookie;
				}					
			}		
	 
			err = resok->reply.entries->name_attributes.present;
			if (1 != err) {
				ERR("Get Fattr3 failure:%s", clnt_sperrno(err));
				clnt_geterr(hi->sb->clntp, &rerr);
				err = rerr.re_errno;
				goto out;
			}
	    		
			hsi_nfs3_fattr2stat(&resok->reply.entries->						name_attributes.post_op_attr_u.attributes, 									&temp->stbuf);
			
			hrc->next = temp;
			hrc = temp;
			resok->reply.entries = resok->reply.entries->nextentry;
		}
	  
		dir_size += *dircount;
		if (dir_size > maxcount)
			break;
		clnt_freeres(clntp, (xdrproc_t)xdr_readdirplus3res, 
						(char *)&res);

		err = resok->reply.eof;
	}while(err == 0);
 
	err = 0;
	hrc = temp1;
	
out:
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
