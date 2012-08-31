
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "hsfs.h"

char *progname = NULL;
char *mountspec = NULL;

static struct fuse_lowlevel_ops hsfs_oper = {
	.init		= hsx_fuse_init,
	.destroy		= hsx_fuse_destroy,
};

static int 
hsi_fuse_parse_cmdline(int argc, char **argv, struct fuse_args *args, char **udata)
{

	return 0;
}

void hsi_fuse_mount_usage()
{
	printf("usage: %s remotetarget dir [-rvVwfnh] [-t version] [-o enfsoptions]\n", progname);
	printf("options:\n\t-r\t\tMount file system readonly\n");
	printf("\t-v\t\tVerbose\n");
	printf("\t-V\t\tPrint version\n");
	printf("\t-w\t\tMount file system read-write\n");
	printf("\t-f\t\tFake mount, don't actually mount\n");
	printf("\t-n\t\tDo not update /etc/mtab\n");
	printf("\t-s\t\tTolerate sloppy mount options rather than failing.\n");
	printf("\t-h\t\tPrint this help\n");
	printf("\tversion\t\tenfs - currently, the only choice\n");
	printf("\tenfsoptions\tRefer mount.enfs(8) or enfs(5)\n\n");
}

int main(int argc, char **argv)
{
	struct fuse_args args = {};
	struct fuse_chan *ch = NULL;
	char *mountpoint = NULL, *userdata = NULL;
	int err = -1;

	progname = basename(argv[0]);

	if (argc < 3) {
		hsi_fuse_mount_usage();
		exit(-1);
	}

	mountspec = argv[1];
	mountpoint = argv[2];
	argc -= 3;

	if (hsi_fuse_parse_cmdline(argc, &argv[3], &args, &userdata) != -1 &&
		(ch = fuse_mount(mountpoint, &args)) != NULL) {
		struct fuse_session *se;

		se = fuse_lowlevel_new(&args, &hsfs_oper,
				       sizeof(hsfs_oper), userdata);
		if (se != NULL) {
			if (fuse_set_signal_handlers(se) != -1) {
				fuse_session_add_chan(se, ch);
				err = fuse_session_loop(se);
				fuse_remove_signal_handlers(se);
				fuse_session_remove_chan(ch);
			}
			fuse_session_destroy(se);
		}
		fuse_unmount(mountpoint, ch);
	}
	fuse_opt_free_args(&args);

	return err ? 1 : 0;
}
