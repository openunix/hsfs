/*hsi_nfs3_write.c*/

#include <errno.h>
#include "hsi_nfs3.h"
#include "log.h"

int hsi_nfs3_write(struct hsfs_rw_info* winfo)
{
	struct hsfs_super *sb = winfo->inode->sb;
	CLIENT *clnt = sb->clntp;
	struct write3args args;
	struct write3res res;
	struct write3resok * resok = NULL;
	int err = 0;

	DEBUG_IN("offset 0x%x size 0x%x", (unsigned int)winfo->rw_off,
		(unsigned int)winfo->rw_size);
	memset(&args, 0, sizeof(args));
	memset(&res, 0, sizeof(res));
	args.file.data.data_len = winfo->inode->fh.data.data_len;
	args.file.data.data_val = winfo->inode->fh.data.data_val;
	args.data.data_len = winfo->data.data_len;
	args.data.data_val = winfo->data.data_val;
	args.offset = winfo->rw_off;
	args.count = winfo->rw_size;
	args.stable = winfo->stable;

	err = hsi_nfs3_clnt_call(sb, NFSPROC3_WRITE,
			(xdrproc_t)xdr_write3args, (char *)&args, 
			(xdrproc_t)xdr_write3res, (char *)&res);
	if(err)
		goto out;

#ifdef HSFS_NFS3_TEST
	err = res.status;
#else
	err = hsi_nfs3_stat_to_errno(res.status);
#endif
	if(!err){
		resok = &res.write3res_u.resok;
		winfo->ret_count = resok->count;
		DEBUG("hsi_nfs3_write 0x%x done", resok->count);
		DEBUG("resok->file_wcc.after.present: %d", 
			resok->file_wcc.after.present);
		if(resok->file_wcc.after.present)
			memcpy(&winfo->inode->attr,
				&resok->file_wcc.after.post_op_attr_u.attributes,
				sizeof(fattr3));
	}else{
		ERR("hsi_nfs3_write failure: %d", err);
		DEBUG("res.write3res_u.resfail.after.present: %d", 
			res.write3res_u.resfail.after.present);
		if(res.write3res_u.resfail.after.present)
			memcpy(&winfo->inode->attr,
				&res.write3res_u.resfail.after.post_op_attr_u.attributes,
				sizeof(fattr3));
	}

	clnt_freeres(clnt, (xdrproc_t)xdr_write3res, (char *)&res);

out:
	DEBUG_OUT("err %d", err);
	return err;
}

#ifdef HSFS_NFS3_TEST

int main(int argc, char *argv[])
{
	struct hsfs_rw_info winfo;
	struct hsfs_inode inode;
	struct hsfs_super sblock;
	char *svraddr = NULL;
	char *fpath = NULL;
	char *mpoint = NULL;
	char *buffer = NULL;
	CLIENT *clntp = NULL;
	size_t fhlen = 0 ;
	unsigned char *fhvalp = NULL;
	size_t iosize = 0;
	size_t fsize = 0;
	int err = 0;
	size_t i = 0;
	
	if(argc != 6){
		printf("USEAGE: hsi_nfs3_write  SERVER_IP   FILEPATH  IOSIZE FILESIZE"
			"MOUNTPOINT\n");
		printf("EXAMPLE: ./hsi_nfs3_write    10.10.99.120    "
			"/nfsXport/file   512   1000000  /mnt/hsfs\n");
		err = EINVAL;
		goto out;
	}
	
	svraddr = argv[1];
	fpath = argv[2];
	iosize = atoi(argv[3]);
	fsize = atoi(argv[4]);
	if(0 == iosize | 0 == fsize)
		printf("IOSIZE or FILESIZE is zero.\n");
	mpoint = argv[5];
	clntp = clnt_create(svraddr, NFS_PROGRAM, NFS_V3, "TCP");
	if(NULL == clntp){
		printf("Create handle to RPC server (%s, %u, %u) failed\n",
			svraddr, NFS_PROGRAM, NFS_V3);
		err = EREMOTEIO;
		goto out;
	}

	err = map_path_to_nfs3fh(svraddr, fpath, &fhlen, &fhvalp);
	if(err){
		printf("map_path_to_nfs3fh error: %s.\n", strerror(err));
		err = ENXIO;		
		goto out;
	}
	
	memset(&inode, 0, sizeof(struct hsfs_inode));
	memset(&winfo, 0, sizeof(struct hsfs_rw_info));
	memset(&sblock, 0, sizeof(struct hsfs_super));
	inode.sb = &sblock;
	winfo.inode = &inode;
	
	sblock.timeo = 1200;//120sec
	sblock.clntp = clntp;
	inode.fh.data.data_len = fhlen;
	inode.fh.data.data_val = fhvalp;
	
	winfo.rw_off = 0;
	winfo.rw_size = iosize;
	buffer = (char *) malloc(iosize);
	if( NULL == buffer){
		printf("insufficient memory.\n");
		err = ENOMEM;
		goto out;
	}
	winfo.stable = HSFS_FILE_SYNC;
	memset(buffer, '0', iosize);
	winfo.data.data_len = iosize;
	winfo.data.data_val = buffer;
	for(i = 0; i < fsize; ){
		winfo.rw_off += winfo.ret_count;
		err = hsi_nfs3_write(&winfo);
		if(err)
			break;
		
		i += winfo.ret_count;
	}
out:
	if(NULL != buffer)
		free(buffer);

	return err;
}

#endif /*HSFS_NFS3_TEST*/
