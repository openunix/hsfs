/*hsi_nfs3_read.c*/

#include <errno.h>
#include "hsi_nfs3.h"
#include "log.h"

int hsi_nfs3_read(struct hsfs_rw_info* rinfo)
{
	struct hsfs_super *sb = rinfo->inode->sb;
	CLIENT *clnt = sb->clntp;
	struct read3args args;
	struct read3res res;
	struct read3resok * resok = NULL;
	int err = 0;

	DEBUG_IN("offset 0x%x size 0x%x", (unsigned int)rinfo->rw_off,
		(unsigned int)rinfo->rw_size);
	memset(&args, 0, sizeof(args));
	memset(&res, 0, sizeof(res));
	args.file.data.data_len = rinfo->inode->fh.data.data_len;
	args.file.data.data_val = rinfo->inode->fh.data.data_val;
	args.offset = rinfo->rw_off;
	args.count = rinfo->rw_size;

	err = hsi_nfs3_clnt_call(sb, clnt, NFSPROC3_READ,
				(xdrproc_t)xdr_read3args, (char *)&args,
				(xdrproc_t)xdr_read3res, (char *)&res);
	if (err)
		goto out;

#ifdef HSFS_NFS3_TEST
	err = res.status;
#else
	err = hsi_nfs3_stat_to_errno(res.status);
#endif
	if(!err){
		resok = &res.read3res_u.resok;
		DEBUG("hsi_nfs3_read 0x%x done eof: %d",
				resok->count, resok->eof);
		memcpy(rinfo->data.data_val, resok->data.data_val,
			resok->data.data_len);
		rinfo->data.data_len = resok->data.data_len;
		rinfo->ret_count = resok->count;
		rinfo->eof = resok->eof;
		DEBUG("resok->file_attributes.present: %d",
			resok->file_attributes.present);
		if(resok->file_attributes.present)
			memcpy(&rinfo->inode->attr,
				&resok->file_attributes.post_op_attr_u.attributes,
				sizeof(fattr3));
	}else{
		ERR("hsi_nfs3_read failure: %d", err);
		if(res.read3res_u.resfail.present)
			memcpy(&rinfo->inode->attr,
				&res.read3res_u.resfail.post_op_attr_u.attributes,
				sizeof(fattr3));
	}
	
	clnt_freeres(clnt, (xdrproc_t)xdr_read3res, (char *)&res);

out:
	DEBUG_OUT("err %d", err);
	return err;
}

#ifdef HSFS_NFS3_TEST

int main(int argc, char *argv[])
{
	struct hsfs_rw_info rinfo;
	struct hsfs_inode inode;
	struct hsfs_super sblock;
	char *svraddr = NULL;
	char *fpath = NULL;
	char *mpoint = NULL;
	CLIENT *clntp = NULL;
	size_t fhlen = 0 ;
	unsigned char *fhvalp = NULL;
	size_t iosize = 0;
	int err = 0;
	
	if(argc != 5){
		printf("USEAGE: hsi_nfs3_read  SERVER_IP   FILEPATH   IOSIZE   "
			"MOUNTPOINT\n");
		printf("EXAMPLE: ./hsi_nfs3_read    10.10.99.120   "
			"/nfsXport/file   512  /mnt/hsfs\n");
		err = EINVAL;
		goto out;
	}
	
	svraddr = argv[1];
	fpath = argv[2];
	iosize = atoi(argv[3]);
	if(0 == iosize)
		printf("IOSIZE is zero\n");
	mpoint = argv[4];
	clntp = clnt_create(svraddr, NFS_PROGRAM, NFS_V3, "TCP");
	if(NULL == clntp) {
		printf("Create handle to RPC server (%s, %u, %u) failed\n",
			svraddr, NFS_PROGRAM, NFS_V3);
		err = EREMOTEIO;
		goto out;
	}

	err = map_path_to_nfs3fh(svraddr, fpath, &fhlen, &fhvalp);
	if(err)
	{
		printf("map_path_to_nfs3fh error: %s.\n", strerror(err));
		err = ENXIO;		
		goto out;
	}
	
	memset(&inode, 0, sizeof(struct hsfs_inode));
	memset(&rinfo, 0, sizeof(struct hsfs_rw_info));
	memset(&sblock, 0, sizeof(struct hsfs_super));
	inode.sb = &sblock;
	rinfo.inode = &inode;
	
	sblock.timeo = 1200;//120sec
	sblock.clntp = clntp;
	inode.fh.data.data_len = fhlen;
	inode.fh.data.data_val = fhvalp;
	
	rinfo.rw_off = 0;
	rinfo.rw_size = iosize;

	while(1){
		rinfo.rw_off += rinfo.ret_count;
		err = hsi_nfs3_read(&rinfo);
		if(err)
			break;
		
		if(rinfo.eof)
			break;		
	}

out:
	return err;
}

#endif /*HSFS_NFS3_TEST*/
