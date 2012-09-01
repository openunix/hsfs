
#define FUSE_USE_VERSION 26
#include <fuse_lowlevel.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/user.h>
#include <libgen.h>
#include <getopt.h>
#include <errno.h>

#include "log.h"
#include "config.h"
#include "xcommon.h"
#include "mount_constants.h"

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
	printf("%s.\n", PACKAGE_STRING);
	exit(0);
}

static inline int hsi_fuse_add_opt(struct fuse_args *args, const char *opt)
{
	char **opts = args->argv + (args->argc -1);
	return fuse_opt_add_opt(opts, opt);
}

static struct fuse_lowlevel_ops hsfs_oper = {
};

/*
 * Map from -o and fstab option strings to the flag argument to mount(2).
 */
struct opt_map {
	const char *nfs_opt;	/* option name */
	const char *fuse_opt;
};

static const struct opt_map opt_map[] = {
	{ "defaults",	NULL },		/* default options */
	{ "ro",		"ro" },		/* read-only */
	{ "rw",		"rw" },		/* read-write */
	{ "exec",		"exec" },		/* permit execution of binaries */
	{ "noexec",	"noexec" },	/* don't execute binaries */
	{ "suid",		"suid" },		/* honor suid executables */
	{ "nosuid",	"nosuid" },	/* don't honor suid executables */
	{ "dev", 		"dev" },		/* interpret device files  */
	{ "nodev",	"nodev" },	/* don't interpret devices */
	{ "sync",		"sync" },		/* synchronous I/O */
	{ "async",	"async" },	/* asynchronous I/O */
	{ "dirsync",	"dirsync" },	/* synchronous directory modifications */
	{ "remount",	NULL },		/* Alter flags of mounted FS */
	{ "bind",		NULL },		/* Remount part of tree elsewhere */
	{ "rbind",		NULL },		/* Idem, plus mounted subtrees */
	{ "auto",		NULL },		/* Can be mounted using -a */
	{ "noauto",	NULL },		/* Can  only be mounted explicitly */
	{ "users",	"allow_other" },	/* Allow ordinary user to mount */
	{ "nousers",	"allow_root" },	/* Forbid ordinary user to mount */
	{ "user",		"allow_other" },	/* Allow ordinary user to mount */
	{ "nouser",	"allow_root" },	/* Forbid ordinary user to mount */
	{ "owner",	NULL },		/* Let the owner of the device mount */
	{ "noowner",	NULL },		/* Device owner has no special privs */
	{ "group",	NULL },		/* Let the group of the device mount */
	{ "nogroup",	NULL },		/* Device group has no special privs */
	{ "_netdev",	NULL },		/* Device requires network */
	{ "comment",	NULL },		/* fstab comment only (kudzu,_netdev)*/

	/* add new options here */
#ifdef MS_NOSUB
	{ "sub",		NULL },		/* allow submounts */
	{ "nosub",	NULL },		/* don't allow submounts */
#endif
#ifdef MS_SILENT
	{ "quiet",		NULL },		/* be quiet  */
	{ "loud",		NULL },		/* print out messages. */
#endif
#ifdef MS_MANDLOCK
	{ "mand",	NULL },		/* Allow mandatory locks on this FS */
	{ "nomand",	NULL },		/* Forbid mandatory locks on this FS */
#endif
	{ "loop",		NULL },		/* use a loop device */
#ifdef MS_NOATIME
	{ "atime",	"atime" },	/* Update access time */
	{ "noatime",	"noatime" },	/* Do not update access time */
#endif
#ifdef MS_NODIRATIME
	{ "diratime",	NULL },		/* Update dir access times */
	{ "nodiratime",	NULL },		/* Do not update dir access times */
#endif
#ifdef MS_RELATIME
	{ "relatime",	NULL },		/* Update access times relative to
						mtime/ctime */
	{ "norelatime",	NULL },		/* Update access time without regard
						to mtime/ctime */
#endif
	{ "noquota",	NULL },        /* Don't enforce quota */
	{ "quota", 	NULL },          /* Enforce user quota */
	{ "usrquota", 	NULL },       /* Enforce user quota */
	{ "grpquota", 	NULL },       /* Enforce group quota */
	{ NULL,		NULL }
};

static void hsi_parse_opt(const char *opt, struct fuse_args *args,
					char *extra_opts, size_t len)
{
	const struct opt_map *om = NULL;

	for (om = opt_map; om->nfs_opt != NULL; om++) {
		if (!strcmp (opt, om->nfs_opt)) {
			if (om->fuse_opt) {
				hsi_fuse_add_opt(args, om->fuse_opt);
			} else {
				WARNING("Not supported opt: %s.", opt);
			}
			return;
		}
	}

	len -= strlen(extra_opts);

	if (*extra_opts && --len > 0)
		strcat(extra_opts, ",");

	if ((len -= strlen(opt)) > 0)
		strcat(extra_opts, opt);
}


static void hsi_parse_opts(const char *options, struct fuse_args *args,
							char **udata)
{
	if (options != NULL) {
		char *opts = xstrdup(options);
		char *opt = NULL, *p = NULL;
		size_t len = strlen(opts) + 1;	/* include room for a null */
		int open_quote = 0;

		*udata = xmalloc(len);
		**udata = '\0';

		for (p = opts, opt = NULL; p && *p; p++) {
			if (!opt)
				opt = p;	/* begin of the option item */
			if (*p == '"')
				open_quote ^= 1; /* reverse the status */
			if (open_quote)
				continue;	/* still in a quoted block */
			if (*p == ',')
				*p = '\0';	/* terminate the option item */

			/* end of option item or last item */
			if (*p == '\0' || *(p + 1) == '\0') {
				hsi_parse_opt(opt, args, *udata, len);
				opt = NULL;
			}
		}
		free(opts);
	}
}

static int hsi_parse_cmdline(int argc, char **argv, struct fuse_args *args,
							char **udata)
{
	char *tdata = NULL;
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
			tdata = optarg;
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

	hsi_parse_opts(tdata, args, udata);

	ret = fuse_daemonize(foreground);
out:
	return ret;
}

int main(int argc, char **argv)
{
	struct fuse_args args = {};
	struct fuse_chan *ch = NULL;
	char *mountpoint = NULL, *udata = NULL, *userdata = NULL;
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
	if ((hsi_parse_cmdline(argc - 2, argv + 2, &args, &udata)) != -1 &&
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
