#ifdef HSFS_NFS3_TEST
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include "hsi_nfs3.h"
#include "hsx_fuse.h"
#include "apis.h"


struct filetype{
	int type;
	char file_name[20]; 
};
struct modetype{
	int mode;
	char mode_name[20];
};
int main(int argc,char **argv)
{
	struct filetype typetest[]={ 	{S_IFCHR, "-CHR"},
					{S_IFBLK, "-BLK"},
					{S_IFSOCK, "-SOCK"},
					{S_IFIFO, "-FIFO"},
					{S_IFREG, "-REG"},
					{S_IFDIR, "-DIR"},
					{S_IFLNK, "-LNK"}};
	struct modetype modetest[]={	{0x00000700, "-HIGH"},
					{0x000001c0, "-USR"},
					{0x00000038, "-GRO"},
					{0x00000007, "-OTH"}};
	CLIENT *clntp=NULL;
	int k;
	for(k=0;k<7;k++){
		printf(" %s , %x \n",typetest[k].file_name,typetest[k].type);
	}
	printf(" %s , %x \n","S_ISUID",S_ISUID);
	printf(" %s , %x \n","S_ISGID",S_ISGID);
	printf(" %s , %x \n","S_ISVTX",S_ISVTX);
	printf(" %s , %x \n","S_IRWXU",S_IRWXU);
	printf(" %s , %x \n","S_IRWXG",S_IRWXG);
	printf(" %s , %x \n","S_IRWXO",S_IRWXO);
		
	

	struct mknod3args args;
	struct hsfs_inode *parent=NULL;
	struct hsfs_inode *newinode=NULL;
	struct diropres3 res;
	struct timeval to={120,0};
	nfs_fh3 nfs3_fh;
	char *host;
	char *fpath;
	enum clnt_stat st;
	char *cliname=basename(argv[0]);
	int err=0;
	if(argc < 4) {
		err=EINVAL;
		printf("usage: %s $server_host $f_path $name \n",cliname);
		goto out;
	}
	host = argv[1];
	fpath = argv[2];
	char *name = argv[3];

	clntp=clnt_create(host,NFS_PROGRAM,NFS_V3,"TCP");
	if( NULL == clntp){
		printf("err occur when create client \n");
		err=ENXIO;
		goto out;
	}
	memset(& args, 0 , sizeof( args ));
	memset(& res, 0 , sizeof( res ));
	memset(& nfs3_fh , 0 , sizeof( nfs3_fh ));
	printf("host: %s , f_path: %s \n", host, fpath);
	err=map_path_to_nfs3fh(host,fpath,&nfs3_fh.data.data_len,&nfs3_fh.data.data_val);
	if(err){
		printf(" map path to nfs3fh failed \n");
		printf("%d \n",err);
		goto out;
	}
	printf("map down \n");
	parent=(struct hsfs_inode *)malloc(sizeof(struct hsfs_inode));

	if(!parent){
		printf("error when malloc a hsfs_inode: \n");
		goto out;
	}
	parent->fh.data.data_len=nfs3_fh.data.data_len;
	parent->fh.data.data_val=nfs3_fh.data.data_val;

	parent->sb=(struct hsfs_super*)malloc(sizeof(struct hsfs_super));
	parent->sb->clntp=clntp;
	dev_t rdev=MKDEV(8,1);
	int i,j;
	for(i=0;i<7;i++){
		int mode=0;
		char *namep=(char *)malloc(50*sizeof(char));
		strcpy(namep,name);
		strcat(namep,typetest[i].file_name);
		mode|=typetest[i].type;
		for(j=0;j<4;j++){
			int mode2=mode;
			char *namep2=(char *)malloc(50*sizeof(char));
			strcpy(namep2,namep);
			strcat(namep2,modetest[j].mode_name);
			mode2|=modetest[j].mode;	
			printf(" name : %s, mode : %x \n",namep2,mode2);

			err=hsi_nfs3_mknod(parent,&newinode,namep2,mode2,rdev);
			
			printf("err number :%d \n",err);
			free(namep2);
		}
		free(namep);
	}
out:	
	free(parent->sb);
	free(parent);
	getchar();
	return err;	
}
#else 
#include <errno.h>
#include "hsi_nfs3.h"
#endif /*#ifdef HSFS_NFS3_TEST*/

int
hsi_nfs3_mknod(struct hsfs_inode *parent, struct hsfs_inode **new,
               const char *name,mode_t mode, dev_t rdev)
{
	struct hsfs_super *sb = parent->sb;
	int err=0;
	CLIENT *nfs_client=NULL;
	struct mknod3args args;
	struct diropres3 res;

	DEBUG_IN("the parent ino (%lu),the nlookup is (%lu)",
		 parent->ino, (unsigned long)parent->private);

	memset(&args, 0 , sizeof(struct mknod3args));
	memset(&res, 0 , sizeof(struct diropres3));	

	if(strlen(name) > NAME_MAX){
		ERR("%s  is too long \n",name);
		err=ENAMETOOLONG;
		goto out2;
	}
	// get the fh
	hsi_nfs3_getfh3(parent, &args.where.dir);
	// alloc memory for name
	args.where.name=(char *)malloc(NAME_MAX * sizeof(char));

	if(!args.where.name){
		err=ENOMEM;
		goto out2;
	}

	memset(args.where.name, 0 ,NAME_MAX );
	strcpy(args.where.name,name);

	// determin the type of the file
	switch(mode&S_IFMT){
		case S_IFCHR:
			args.what.type=NF3CHR;
			goto sattrs;
		case S_IFBLK:
			args.what.type=NF3BLK;
sattrs:
			args.what.mknoddata3_u.device.dev_attributes.mode.set=1;
			args.what.mknoddata3_u.device.spec.major=major(rdev);
			args.what.mknoddata3_u.device.spec.minor=minor(rdev);
			args.what.mknoddata3_u.device.dev_attributes.mode.
						set_uint32_u.val=mode & 0xfff;

			break;
		case S_IFSOCK:
			args.what.type=NF3SOCK;
			goto pipe_sattrs;
			break;
		case S_IFIFO:
			args.what.type=NF3FIFO;
pipe_sattrs:
			args.what.mknoddata3_u.pipe_attributes.mode.set=1;
			args.what.mknoddata3_u.pipe_attributes.mode.
					set_uint32_u.val=mode & 0xfff;

			break;
		default : break;
	}
	// get the client 
	nfs_client=parent->sb->clntp; 
	memset(&res,0,sizeof(res));
	err=hsi_nfs3_clnt_call(parent->sb,nfs_client,NFSPROC3_MKNOD,
				(xdrproc_t)xdr_mknod3args,(caddr_t)&args,
				(xdrproc_t)xdr_diropres3,(caddr_t)&res);
	if(err)
		goto out2;

	if(res.status){
		ERR("Call NFS3 Server Failure:(%d).\n",res.status);
		err=hsi_nfs3_stat_to_errno(res.status);
		goto out1;
	}

	*new = hsi_nfs3_handle_create(sb, &res.diropres3_u.resok);
	if(IS_ERR(*new)){
		*new = NULL;
		err = PTR_ERR(*new);
	}
out1:
	clnt_freeres(nfs_client,(xdrproc_t)xdr_diropres3,(char *)&res);
out2:
	if(args.where.name)
		free(args.where.name);

	DEBUG_OUT(" err: %d",err);
	return err;	
}
