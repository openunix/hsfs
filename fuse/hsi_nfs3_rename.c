/**
 * hsi_nfs3_rename.c
 * yanhuan
 * 2012.9.6
 **/

#define TEST_RENAME

#ifdef TEST_RENAME
# include <rpc/rpc.h>
# include <libgen.h>
# include <errno.h>
# include <arpa/inet.h>		/* inet_ntoa */
# include "nfs3.h"
# include "../include/log.h"
# include "apis.h"

char *progname;
char *svraddr;
struct hsfs_super {
	CLIENT			*clntp;
	int			timeo;
	struct sockaddr_in	addr;
};
struct hsfs_inode
{
	uint64_t		ino;
	unsigned long 		generation;
	nfs_fh3			fh;
	unsigned long		nlookup;
	fattr3_new		attr;
	struct hsfs_super	*sb;
	struct hsfs_inode 	*next;
};
#else
# include "../include/hsi_nfs3.h"
#endif

extern int hsi_nfs3_rename(struct hsfs_inode *hi, const char *name,
		 		struct hsfs_inode *newhi, const char *newname)
{
	int ret = 0;
	int err = 0;
	rename3args args;
	rename3res res;
	struct timeval to;
	to.tv_sec = hi->sb->timeo / 10;
	to.tv_usec = (hi->sb->timeo % 10) * 100;
	memset(&args, 0, sizeof(args));
	memset(&res, 0, sizeof(res));
	args.from.dir.data.data_len = hi->fh.data.data_len;
	args.from.dir.data.data_val = hi->fh.data.data_val;
	args.from.name = name;
	args.to.dir.data.data_len = newhi->fh.data.data_len;
	args.to.dir.data.data_val = newhi->fh.data.data_val;
	args.to.name = newname;

	ret = clnt_call(hi->sb->clntp, NFSPROC3_RENAME,
			(xdrproc_t)xdr_rename3args, (caddr_t)&args,
			(xdrproc_t)xdr_rename3res, (caddr_t)&res, to);
	if (ret) {
		ERR("%s: Call RPC Server (%s, %u, %u) failure: "
			"(%s).\n", progname, svraddr,
			NFS_PROGRAM, NFS_V3, clnt_sperrno(ret));
		err = hsi_rpc_stat_to_errno(hi->sb->clntp);
		goto out;
	}
	ret = res.status;
	if (NFS3_OK != ret) {
		ERR("%s: Path (%s) on Server (%s) is not "
			"accessible: (%d).\n", progname, name,
			svraddr, ret);
		err = hsi_nfs3_stat_to_errno(ret);
		goto out;
	}
	else {
		if(res.rename3res_u.res.fromdir_wcc.after.present) {
			memcpy(&(hi->attr), &res.rename3res_u.res.fromdir_wcc.
				after.post_op_attr_new_u.
				attributes, sizeof(fattr3_new));
		}
		if(res.rename3res_u.res.todir_wcc.after.present) {
			memcpy(&(newhi->attr), &res.rename3res_u.res.todir_wcc.
				after.post_op_attr_new_u.
				attributes, sizeof(fattr3_new));
		}
	}
out:
	return err;

}

#ifdef TEST_RENAME
int main(int argc, char *argv[])
{
	int err = 0;
	struct hsfs_super sb;
	progname = basename(argv[0]);
	sb.addr.sin_addr.s_addr = inet_addr("10.10.211.154");
	svraddr = inet_ntoa(*(struct in_addr *)&sb.addr.sin_addr);
	sb.clntp = clnt_create(svraddr,	NFS_PROGRAM, NFS_V3, "udp");
	if (NULL == sb.clntp) {
		fprintf(stderr, "%s: Create handle to RPC server "
			"(%s, %u, %u) failed: (%s).\n", progname,
			svraddr, NFS_PROGRAM, NFS_V3,
			clnt_spcreateerror(progname));
		err = ENXIO;
		goto out;
	}
	char *fpath = "/rpcdir/";
	unsigned char *nfs3fhp = NULL;
	size_t fhlen;
	if ((err = map_path_to_nfs3fh(svraddr, fpath, &fhlen, &nfs3fhp))){
		fprintf(stderr, "%s: Mapping %s on server to its FH feiled.%d\n",
			progname, fpath, err);
		goto out;
	}
	struct hsfs_inode *hi = NULL;
	if (!(hi = (struct hsfs_inode *)malloc(sizeof(struct hsfs_inode)))) {
		fprintf(stderr, "%s: No enough core\n", progname);
	}
	sb.timeo = 10;
	hi->sb = &sb;
	hi->fh.data.data_len = fhlen;
	hi->fh.data.data_val = nfs3fhp;

	fpath = "/rpcdir/test/";
	unsigned char *nfs3fhp2 = NULL;
	if ((err = map_path_to_nfs3fh(svraddr, fpath, &fhlen, &nfs3fhp2))){
		fprintf(stderr, "%s: Mapping %s on server to its FH feiled.%d\n",
			progname, fpath, err);
		goto out;
	}
	struct hsfs_inode *newhi = NULL;
	if (!(newhi = (struct hsfs_inode *)malloc(sizeof(struct hsfs_inode)))) {
		fprintf(stderr, "%s: No enough core\n", progname);
	}
	newhi->sb = &sb;
	newhi->fh.data.data_len = fhlen;
	newhi->fh.data.data_val = nfs3fhp2;

	hsi_nfs3_rename(hi, argv[1], newhi, argv[2]);
out:
	exit(err);
}
#endif
