
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/user.h>
#include <libgen.h>
#include <getopt.h>
#include <errno.h>

#include "log.h"
#include "hsfs.h"
#include "config.h"
#include "xcommon.h"
#include "mount_constants.h"

char *progname = NULL;
int verbose = 0;
int nomtab = 0;
int fg = 0;

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
	int skip;		/* skip in mtab option string */
	int inv;		/* true if flag value should be inverted */
	int mask;		/* flag mask value */
	const char *fuse_opt;	/* for fuse args mapping */
};

/* Custom mount options for our own purposes.  */
/* Maybe these should now be freed for kernel use again */
#define MS_DUMMY	0x00000000
#define MS_USERS	0x40000000
#define MS_USER		0x20000000

static const struct opt_map opt_map[] = {
  { "defaults", 0, 0, 0              , NULL      },    /* default options */
  { "ro",       1, 0, MS_RDONLY      , "ro"      },    /* read-only */
  { "rw",       1, 1, MS_RDONLY      , "rw"      },    /* read-write */
  { "exec",     0, 1, MS_NOEXEC      , "exec"    },    /* permit execution of binaries */
  { "noexec",   0, 0, MS_NOEXEC      , "noexec"  },    /* don't execute binaries */
  { "suid",     0, 1, MS_NOSUID      , "suid"    },    /* honor suid executables */
  { "nosuid",   0, 0, MS_NOSUID      , "nosuid"  },    /* don't honor suid executables */
  { "dev",      0, 1, MS_NODEV       , "dev"     },    /* interpret device files  */
  { "nodev",    0, 0, MS_NODEV       , "nodev"   },    /* don't interpret devices */
  { "sync",     0, 0, MS_SYNCHRONOUS , "sync"    },    /* synchronous I/O */
  { "async",    0, 1, MS_SYNCHRONOUS , "async"   },    /* asynchronous I/O */
  { "dirsync",  0, 0, MS_DIRSYNC     , "dirsync" },    /* synchronous directory modifications */
  { "remount",  0, 0, MS_REMOUNT     , NULL      },    /* Alter flags of mounted FS */
  { "bind",     0, 0, MS_BIND        , NULL      },    /* Remount part of tree elsewhere */
  { "rbind",    0, 0, MS_BIND|MS_REC , NULL      },    /* Idem, plus mounted subtrees */
  { "auto",     0, 0, MS_DUMMY       , NULL      },    /* Can be mounted using -a */
  { "noauto",   0, 0, MS_DUMMY       , NULL      },    /* Can  only be mounted explicitly */
  { "users",    0, 0, MS_USERS       , "allow_other" },/* Allow ordinary user to mount */
  { "nousers",  0, 1, MS_USERS       , "allow_root"  },/* Forbid ordinary user to mount */
  { "user",     0, 0, MS_USER        , "allow_other" },/* Allow ordinary user to mount */
  { "nouser",   0, 1, MS_USER        , "allow_root"  },/* Forbid ordinary user to mount */
  { "owner",    0, 0, MS_DUMMY       , NULL      },    /* Let the owner of the device mount */
  { "noowner",  0, 0, MS_DUMMY       , NULL      },    /* Device owner has no special privs */
  { "group",    0, 0, MS_DUMMY       , NULL      },    /* Let the group of the device mount */
  { "nogroup",  0, 0, MS_DUMMY       , NULL      },    /* Device group has no special privs */
  { "_netdev",  0, 0, MS_DUMMY       , NULL      },    /* Device requires network */
  { "comment",  0, 0, MS_DUMMY       , NULL      },    /* fstab comment only (kudzu,_netdev)*/

  /* add new options here */
#ifdef MS_NOSUB
  { "sub",      0, 1, MS_NOSUB       , NULL      },    /* allow submounts */
  { "nosub",    0, 0, MS_NOSUB       , NULL      },    /* don't allow submounts */
#endif
#ifdef MS_SILENT
  { "quiet",    0, 0, MS_SILENT      , NULL      },    /* be quiet  */
  { "loud",     0, 1, MS_SILENT      , NULL      },    /* print out messages. */
#endif
#ifdef MS_MANDLOCK
  { "mand",     0, 0, MS_MANDLOCK    , NULL      },    /* Allow mandatory locks on this FS */
  { "nomand",   0, 1, MS_MANDLOCK    , NULL      },    /* Forbid mandatory locks on this FS */
#endif
  { "loop",     1, 0, MS_DUMMY       , NULL      },    /* use a loop device */
#ifdef MS_NOATIME
  { "atime",    0, 1, MS_NOATIME     , "atime"   },     /* Update access time */
  { "noatime",  0, 0, MS_NOATIME     , "noatime" },     /* Do not update access time */
#endif
#ifdef MS_NODIRATIME
  { "diratime", 0, 1, MS_NODIRATIME  , NULL      },  /* Update dir access times */
  { "nodiratime", 0, 0, MS_NODIRATIME, NULL      },/* Do not update dir access times */
#endif
  { NULL,	0, 0, 0		}
};

static void hsi_parse_opt(const char *opt, int *flags, struct fuse_args *args,
					char *extra_opts, size_t len)
{
	const struct opt_map *om = NULL;

	for (om = opt_map; om->nfs_opt != NULL; om++) {
		if (!strcmp (opt, om->nfs_opt)) {
			if (om->fuse_opt) {
				hsi_fuse_add_opt(args, om->fuse_opt);
				if (om->inv)
					*flags &= ~om->mask;
				else
					*flags |= om->mask;
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


static void hsi_parse_opts(const char *options, int *flags, 
				struct fuse_args *args, char **udata)
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
				hsi_parse_opt(opt, flags, args, *udata, len);
				opt = NULL;
			}
		}
		free(opts);
	}
}

static int hsi_parse_cmdline(int argc, char **argv, int *flags, 
				struct fuse_args *args, char **udata)
{
	char *tdata = NULL;
	int c = 0, ret = 0;

	while((c = getopt_long(argc, argv, "rvVwfno:hs",
			        hsfs_opts, NULL)) != -1) {
		switch(c) {
		case 'r':
			*flags |= MS_RDONLY;
			break;
		case 'v':
			++verbose;
			break;
		case 'V':
			hsi_print_version();
			break;
		case 'w':
			*flags &= ~MS_RDONLY;
			break;
		case 'f':
			fg = 1;
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
		opt = (*flags) & MS_RDONLY ? "ro" : "rw";
		ret = hsi_fuse_add_opt(args, opt);
		if (ret)
			goto out;
	}

	hsi_parse_opts(tdata, flags, args, udata);

	ret = fuse_daemonize(fg);
out:
	return ret;
}

int main(int argc, char **argv)
{
	struct fuse_args args = {};
	struct fuse_chan *ch = NULL;
	struct fuse_session *se = NULL;
	char *mountpoint = NULL, *mountspec = NULL, *udata = NULL;
	struct hsfs_super super = {};
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
	err = hsi_parse_cmdline(argc - 2, argv + 2, &super.flags,
				&args, &udata);
	if (err != 0)
		goto out;

	ch = hsi_fuse_mount(mountspec, mountpoint, &args, udata, &super);
	if (ch == NULL)
		goto out;

	se = fuse_lowlevel_new(&args, &hsfs_oper, sizeof(hsfs_oper), &super);
	if (se != NULL) {
		if (fuse_set_signal_handlers(se) != -1) {
			fuse_session_add_chan(se, ch);
			err = fuse_session_loop(se);
			fuse_remove_signal_handlers(se);
			fuse_session_remove_chan(ch);
		}
		fuse_session_destroy(se);
	}
	hsi_fuse_unmount(mountspec, mountpoint, ch, &super);
out:
	if (udata)
		free(udata);

	fuse_opt_free_args(&args);

	return err ? 1 : 0;
}
