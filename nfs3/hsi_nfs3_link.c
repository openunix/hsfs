/*
 * Copyright (C) 2012 Sun Xiaobin, Feng Shuo <steve.shuo.feng@gmail.com>
 *
 * This file is part of HSFS.
 *
 * HSFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * HSFS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HSFS.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HSFS_NFS3_TEST

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/kdev_t.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include "hsi_nfs3.h"
#include "hsx_fuse.h"
#include "apis.h"


int main(int argc,char **argv)
{
	CLIENT *clntp=NULL;
	struct link3args args;
	struct hsfs_inode *parent=NULL;
	struct hsfs_inode *ino=NULL;
	struct hsfs_inode *newinode=NULL;
	struct link3res res;
	struct timeval to={120,0};
	nfs_fh3 newp_fh;
	nfs_fh3 ino_fh;
	char *host;
	char *newp_path;
	char *ino_path;
	enum clnt_stat st;
	char *cliname=basename(argv[0]);
	int err=0;
	if(argc < 5) {
		err=EINVAL;
		printf("usage: %s $server_host, $ino_path, $newparent_path, $name \n",cliname);
		goto out;
	}
	host = argv[1];
	ino_path = argv[2];
	newp_path=argv[3];
	char *name = argv[4];

	clntp=clnt_create(host,NFS_PROGRAM,NFS_V3,"TCP");
	if( NULL == clntp){
		printf("err occur when create client \n");
		err=ENXIO;
		goto out;
	}
	memset(& args, 0 , sizeof( args ));
	memset(& res, 0 , sizeof( res ));
	memset(&newp_fh , 0 , sizeof( newp_fh ));
	memset(&ino_fh, 0, sizeof(ino_fh));
	err=map_path_to_nfs3fh(host,ino_path,&ino_fh.data.data_len,&ino_fh.data.data_val);
	if(err){
		printf("%s  map path to nfs3fh failed \n",ino_path);
		printf("%d \n",err);
		goto out;
	}
	err=map_path_to_nfs3fh(host,newp_path,&newp_fh.data.data_len,&newp_fh.data.data_val);
	if(err){
		printf("%s  map path to nfs3fh failed \n",newp_path);
		printf("%d \n",err);
		goto out;
	}

	printf("map down \n");
	parent=(struct hsfs_inode *)malloc(sizeof(struct hsfs_inode));

	if(!parent){
		printf("error when malloc a hsfs_inode: \n");
		goto out;
	}
	parent->fh.data.data_len=newp_fh.data.data_len;
	parent->fh.data.data_val=newp_fh.data.data_val;

	parent->sb=(struct hsfs_super*)malloc(sizeof(struct hsfs_super));
	parent->sb->clntp=clntp;
	ino=(struct hsfs_inode *)malloc(sizeof(struct hsfs_inode));

	if(!ino){
		printf("error when malloc a hsfs_inode: \n");
		goto out;
	}
	ino->fh.data.data_len=ino_fh.data.data_len;
	ino->fh.data.data_val=ino_fh.data.data_val;

	ino->sb=(struct hsfs_super*)malloc(sizeof(struct hsfs_super));
	ino->sb->clntp=clntp;

	fuse_req_t  req=NULL;
	err = hsi_nfs3_link(ino,parent,&newinode,name);
	printf("err number :%d \n",err);
out:	
	getchar();
	return err;	
}
#else
#include <errno.h>
#include "hsi_nfs3.h"
#define MNTNAMLEN 255
#endif

int
hsi_nfs3_link(struct hsfs_inode *inode, struct hsfs_inode *newparent,
	      const char *name)
{
	CLIENT *clntp=NULL;
	int err=0;
	struct link3args args;
	struct link3res res;
	struct nfs_fattr fattr;

	DEBUG_IN("Link from (%lu:%lu) to (%lu:%lu)", inode->ino,
		 (unsigned long)inode->private, newparent->ino,
		 (unsigned long)newparent->private);

	memset(&args, 0, sizeof(args));

	if(strlen(name) > NAME_MAX){
		ERR("%s is too long \n",name);
		err=ENAMETOOLONG;
		goto out2;
	}

	hsi_nfs3_getfh3(inode, &args.file);
	hsi_nfs3_getfh3(newparent, &args.link.dir);

	args.link.name=(char *)malloc(NAME_MAX * sizeof(char));

	if(!args.link.name){
		err=ENOMEM;
		goto out2;
	}
	strcpy(args.link.name, name);
	memset(&res, 0, sizeof(res));

	clntp=newparent->sb->clntp; //get the client	
	err=hsi_nfs3_clnt_call(newparent->sb, clntp, NFSPROC3_LINK,
				(xdrproc_t)xdr_link3args, (caddr_t)&args,
				(xdrproc_t)xdr_link3res, (caddr_t)&res);
	if(err)
		goto out2;

	if(res.status){
		ERR("Call NFS3 Server Failure (%d) \n", res.status);
		err=hsi_nfs3_stat_to_errno(res.status);
		goto out1;
	}

	nfs_init_fattr(&fattr);
	hsi_nfs3_post2fattr(&res.link3res_u.res.file_attributes, &fattr);

	if (!(fattr.valid & NFS_ATTR_FATTR))
		err = hsi_nfs3_getattr(inode, NULL);
	else
		err = nfs_refresh_inode(inode, &fattr);

out1:
	clnt_freeres(clntp,(xdrproc_t)xdr_link3res,(char *)&res);
out2:
	if(args.link.name)
		free(args.link.name);	

	DEBUG_OUT("  err:%d",err);
	return err;
}
