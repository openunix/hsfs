
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/user.h>
#include <libgen.h>
#include <getopt.h>
#include <errno.h>

#include "hsfs.h"

char *progname = NULL;
char *mountspec = NULL;
int verbose = 0;
int nomtab = 0;

static struct option hsfs_opts[] = {
	{ "foreground", 0, 0, 'f' },
	{ "help", 0, 0, 'h' },
	{ "no-mtab", 0, 0, 'n' },
	{ "read-only", 0, 0, 'r' },
	{ "ro", 0, 0, 'r' },
	{ "read-write", 0, 0, 'w' },
	{ "rw", 0, 0, 'w' },
	{ "verbose", 0, 0, 'v' },
	{ "version", 0, 0, 'V' },
	{ "options", 1, 0, 'o' },
	{ NULL, 0, 0, 0 }
};

void hsi_mount_usage()
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
	exit(0);
}

static inline void hsi_print_version(void)
{
	printf("%s: hsfs 0.1.\n", progname);
	exit(0);
}

static inline int hsi_fuse_add_opt(struct fuse_args *args, const char *opt)
{
	char **opts = args->argv + (args->argc -1);
	return fuse_opt_add_opt(opts, opt);
}

static struct fuse_lowlevel_ops hsfs_oper = {
	.init		= hsx_fuse_init,
	.destroy	= hsx_fuse_destroy,
};

static int 
hsi_parse_cmdline(int argc, char **argv, struct fuse_args *args, char **udata)
{
	int flags = 0, c = 0, foreground = 0, ret = 0;

	while((c = getopt_long(argc, argv, "rvVwfno:hs",
			        hsfs_opts, NULL)) != -1) {
		switch(c) {
		case 'r':
			flags |= MS_RDONLY;
			break;
		case 'v':
			++verbose;
			break;
		case 'V':
			hsi_print_version();
			break;
		case 'w':
			flags &= ~MS_RDONLY;
			break;
		case 'f':
			foreground = 1;
			break;
		case 'n':
			nomtab = 1;
			break;
		case 'o':
			*udata = optarg;
			break;
		case 'h':
		default:
			hsi_mount_usage();
			break;
			
		}
	}

	if (optind != argc)
		hsi_mount_usage();

	/* add subtype args */
	{
		char subtype_opt[PAGE_SIZE] = {};

		sprintf(subtype_opt, "-osubtype=%s", progname);
		ret = fuse_opt_add_arg(args, subtype_opt);
		if (ret)
			goto out;
	}

	/* add fs mode */
	{
		char *opt = NULL;
		opt = flags & MS_RDONLY ? "ro" : "rw";
		ret = hsi_fuse_add_opt(args, opt);
		if (ret)
			goto out;
	}

	ret = fuse_daemonize(foreground);
out:
	return ret;
}

int main(int argc, char **argv)
{
	struct fuse_args args = {};
	struct fuse_chan *ch = NULL;
	char *mountpoint = NULL, *userdata = NULL;
	int err = -1;

	progname = basename(argv[0]);

	if (argv[1] && argv[1][0] == '-') {
		if(argv[1][1] == 'V')
			hsi_print_version();
		else
			hsi_mount_usage();
	}

	if (argc < 3)
		hsi_mount_usage();

	mountspec = argv[1];
	mountpoint = argv[2];

	argv[2] = argv[0]; /* so that getopt error messages are correct */
	if ((hsi_parse_cmdline(argc - 2, argv + 2, &args, &userdata)) != -1 &&
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
