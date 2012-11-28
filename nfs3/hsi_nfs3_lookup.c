/*
 * Copyright (C) 2012 Shou Xiaoyun, Feng Shuo <steve.shuo.feng@gmail.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "hsi_nfs3.h"

#ifdef HSFS_NFS3_TEST
#include "apis.h"
struct hsfs_super s;
struct hsfs_super *sb=&s;
#endif /* HSFS_NFS3_TEST */

/*
 * Map file type to S_IFMT bits
 */
static const mode_t nfs_type2fmt[] = {
	[NF3REG] = S_IFREG,
	[NF3DIR] = S_IFDIR,
	[NF3BLK] = S_IFBLK,
	[NF3CHR] = S_IFCHR,
	[NF3LNK] = S_IFLNK,
	[NF3SOCK] = S_IFSOCK,
	[NF3FIFO] = S_IFIFO,
};

void hsi_nfs3_fattr2fattr(struct fattr3 *f, struct nfs_fattr *t)
{
	unsigned int type, minor, major;
	mode_t fmode;
	
	if(f->type > NF3FIFO)
		type = 0;
	else
		type = f->type;
	fmode = nfs_type2fmt[type];
	
	t->mode = (f->mode & ~S_IFMT) | fmode;
	t->nlink = f->nlink;
	t->uid = f->uid;
	t->gid = f->gid;
	t->size = f->size;
	t->du.nfs3.used = f->used;
	
	major = f->rdev.major;
	minor = f->rdev.minor;
	t->rdev = makedev(major, minor);
	if ((unsigned)major(t->rdev) != major ||
	    (unsigned)minor(t->rdev) != minor)
		t->rdev = 0;
	
	t->fsid.major = f->fsid;
	t->fsid.minor = 0;
	t->fileid = f->fileid;
	
	hsi_nfs3_time2spec(&(f->atime), &(t->atime));
	hsi_nfs3_time2spec(&(f->mtime), &(t->mtime));
	hsi_nfs3_time2spec(&(f->ctime), &(t->ctime));
	
	t->valid |= NFS_ATTR_FATTR_V3;
}

int hsi_nfs3_post2fattr(struct post_op_attr *p, struct nfs_fattr *t)
{
	if (p->present)
		hsi_nfs3_fattr2fattr(&(p->post_op_attr_u.attributes), t);
	
	return p->present;
}


int hsi_nfs3_lookup(struct hsfs_inode *parent,struct hsfs_inode **new, 
		    const char *name)
{
	struct hsfs_super *sb = parent->sb;
	struct diropargs3 args;
	struct lookup3res res;

	struct nfs_fattr fattr;
	struct nfs_fh name_fh;
	
	int st = 0, err = 0;

	memset(&args, 0, sizeof(args));
	memset(&res, 0, sizeof(res));

	DEBUG_IN("P_I(%p:%llu)", parent, parent->ino);

	hsi_nfs3_getfh3(parent, &args.dir);

	args.name = (char *)name;
	err=hsi_nfs3_clnt_call(sb, sb->clntp, NFSPROC3_LOOKUP,
			(xdrproc_t)xdr_diropargs3,(caddr_t)&args,
			(xdrproc_t)xdr_lookup3res, (caddr_t)&res);

	if (err) 
		goto out;

	st = res.status;
	if (NFS3_OK != st){
		ERR("Path (%s) on Server is not "
			"accessible: (%d).",name,st);
		err = hsi_nfs3_stat_to_errno(st);
		clnt_freeres(sb->clntp, (xdrproc_t)xdr_lookup3res, 
			(char *)&res);
		goto out;
	}

	nfs_init_fattr(&fattr);
	hsi_nfs3_post2fattr(&res.lookup3res_u.resok.obj_attributes, &fattr);
	nfs_copy_fh3(&name_fh, res.lookup3res_u.resok.object.data.data_len,
		     res.lookup3res_u.resok.object.data.data_val);
	
	*new = hsi_nfs_fhget(parent->sb, &name_fh, &fattr);

	clnt_freeres(sb->clntp, (xdrproc_t)xdr_lookup3res, (char *)&res);
out:
	DEBUG_OUT("with %d, New inode at %p", err, *new);

	return(err);
}

#ifdef HSFS_NFS3_TEST

int main(int argc ,char *argv[])
{
	char *svraddr = NULL;
	char *cliname = NULL;
	CLIENT *clntp = NULL;
	char *fpath = NULL;
	size_t fhlen = 0;
	unsigned char *pnfsfh = NULL;
	
	struct hsfs_inode parent;
        struct hsfs_inode *child;

	int err = 0;

	cliname = basename(argv[0]);

	if (argc < 4)
	{
		err = EINVAL;
		fprintf(stderr, "%s $svraddr $fpath.\n", cliname);
		goto out;
	}
	hsx_fuse_itable_init(sb);
	svraddr = argv[1];
	fpath = argv[2];

	clntp = clnt_create(svraddr, NFS_PROGRAM, NFS_V3, "udp");
	if (NULL == clntp) 
	{
		fprintf(stderr, "%s: Create handle to RPC server "\
			"(%s, %u, %u) failed: (%s).\n", cliname,\
			svraddr, NFS_PROGRAM, NFS_V3, clnt_spcreateerror(cliname));
		err = ENXIO;
		goto out;
	} 
	
		
	if (map_path_to_nfs3fh(svraddr,fpath,&fhlen,&pnfsfh))
	{
                fprintf(stderr, "%s: Transfrom  %s  to FH failed  "\
                        "(%s, %u, %u) .\n", cliname,\
                        fpath,svraddr, NFS_PROGRAM, NFS_V3);
                goto out;
	}
	parent.sb = sb;
	parent.sb->clntp = clntp;
	parent.fh.data.data_val = (char *)pnfsfh;
	parent.fh.data.data_len = fhlen;

	if((err = hsi_nfs3_lookup(&parent,&child,argv[3])))
	{
		fprintf(stderr,"Call hsi_nfs3_lookup function fails,Error"
			"code : %d\n",err);	
		goto out;
	}

	fprintf(stdout, "Attributes of path '%s':\n""\tino=%lu\t\n\t"\
                        "generation=%lu\t\n\t""nlookup=%lu\t\n\t"\
                        "type=%u\t\n\t""mode=%08x\t\n\t"\
                        "nlink=%u\n\tuid=%u\n\tgid=%u\n\t"\
                        "size=%lu\n\tused=%lu\n\t"\
                        "fsid=%lu\n\tfileid=%lu\n",argv[3],child->ino,
                        child->generation,child->nlookup,child->attr.type,
                        child->attr.mode,child->attr.nlink,child->attr.uid,
                        child->attr.gid,child->attr.size,child->attr.used,
                        child->attr.fsid,child->attr.fileid);

out:
	if (NULL != clntp)
		clnt_destroy(clntp);
	exit(err);
}

#endif/* HSFS_NFS3_TEST */


