/*
 * mount.c -- Linux NFS mount
 *
 * Copyright (C) 2006 Amit Gud <agud@redhat.com>
 *
 * - Basic code and wrapper around mount and umount code of NFS.
 *   Based on util-linux/mount/mount.c.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "config.h"
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <getopt.h>
#include <mntent.h>
#include <pwd.h>
#include <sys/stat.h>

#include "fstab.h"
#include "xcommon.h"
#include "mount_constants.h"
#include "nfs_paths.h"

#include "nfs_mount.h"
#include "nfsumount.h"
#include "mount.h"

char *progname;
int nomtab;
int verbose;
int mounttype;
int sloppy;

static struct option longopts[] = {
  { "fake", 0, 0, 'f' },
  { "help", 0, 0, 'h' },
  { "no-mtab", 0, 0, 'n' },
  { "read-only", 0, 0, 'r' },
  { "ro", 0, 0, 'r' },
  { "verbose", 0, 0, 'v' },
  { "version", 0, 0, 'V' },
  { "read-write", 0, 0, 'w' },
  { "rw", 0, 0, 'w' },
  { "options", 1, 0, 'o' },
  { "enfsvers", 1, 0, 't' },
  { "bind", 0, 0, 128 },
  { "replace", 0, 0, 129 },
  { "after", 0, 0, 130 },
  { "before", 0, 0, 131 },
  { "over", 0, 0, 132 },
  { "move", 0, 0, 133 },
  { "rbind", 0, 0, 135 },
  { NULL, 0, 0, 0 }
};

/* Map from -o and fstab option strings to the flag argument to mount(2).  */
struct opt_map {
  const char *opt;              /* option name */
  int  skip;                    /* skip in mtab option string */
  int  inv;                     /* true if flag value should be inverted */
  int  mask;                    /* flag mask value */
};

/* Custom mount options for our own purposes.  */
/* Maybe these should now be freed for kernel use again */
#define MS_DUMMY	0x00000000
#define MS_USERS	0x40000000
#define MS_USER		0x20000000

static const struct opt_map opt_map[] = {
  { "defaults", 0, 0, 0         },      /* default options */
  { "ro",       1, 0, MS_RDONLY },      /* read-only */
  { "rw",       1, 1, MS_RDONLY },      /* read-write */
  { "exec",     0, 1, MS_NOEXEC },      /* permit execution of binaries */
  { "noexec",   0, 0, MS_NOEXEC },      /* don't execute binaries */
  { "suid",     0, 1, MS_NOSUID },      /* honor suid executables */
  { "nosuid",   0, 0, MS_NOSUID },      /* don't honor suid executables */
  { "dev",      0, 1, MS_NODEV  },      /* interpret device files  */
  { "nodev",    0, 0, MS_NODEV  },      /* don't interpret devices */
  { "sync",     0, 0, MS_SYNCHRONOUS},  /* synchronous I/O */
  { "async",    0, 1, MS_SYNCHRONOUS},  /* asynchronous I/O */
  { "dirsync",  0, 0, MS_DIRSYNC},      /* synchronous directory modifications */
  { "remount",  0, 0, MS_REMOUNT},      /* Alter flags of mounted FS */
  { "bind",     0, 0, MS_BIND   },      /* Remount part of tree elsewhere */
  { "rbind",    0, 0, MS_BIND|MS_REC }, /* Idem, plus mounted subtrees */
  { "auto",     0, 0, MS_DUMMY },       /* Can be mounted using -a */
  { "noauto",   0, 0, MS_DUMMY },       /* Can  only be mounted explicitly */
  { "users",    0, 0, MS_USERS  },      /* Allow ordinary user to mount */
  { "nousers",  0, 1, MS_USERS  },      /* Forbid ordinary user to mount */
  { "user",     0, 0, MS_USER   },      /* Allow ordinary user to mount */
  { "nouser",   0, 1, MS_USER   },      /* Forbid ordinary user to mount */
  { "owner",    0, 0, MS_DUMMY  },      /* Let the owner of the device mount */
  { "noowner",  0, 0, MS_DUMMY  },      /* Device owner has no special privs */
  { "group",    0, 0, MS_DUMMY  },      /* Let the group of the device mount */
  { "nogroup",  0, 0, MS_DUMMY  },      /* Device group has no special privs */
  { "_netdev",  0, 0, MS_DUMMY},        /* Device requires network */
  { "comment",  0, 0, MS_DUMMY},        /* fstab comment only (kudzu,_netdev)*/

  /* add new options here */
#ifdef MS_NOSUB
  { "sub",      0, 1, MS_NOSUB  },      /* allow submounts */
  { "nosub",    0, 0, MS_NOSUB  },      /* don't allow submounts */
#endif
#ifdef MS_SILENT
  { "quiet",    0, 0, MS_SILENT    },   /* be quiet  */
  { "loud",     0, 1, MS_SILENT    },   /* print out messages. */
#endif
#ifdef MS_MANDLOCK
  { "mand",     0, 0, MS_MANDLOCK },    /* Allow mandatory locks on this FS */
  { "nomand",   0, 1, MS_MANDLOCK },    /* Forbid mandatory locks on this FS */
#endif
  { "loop",     1, 0, MS_DUMMY   },      /* use a loop device */
#ifdef MS_NOATIME
  { "atime",    0, 1, MS_NOATIME },     /* Update access time */
  { "noatime",  0, 0, MS_NOATIME },     /* Do not update access time */
#endif
#ifdef MS_NODIRATIME
  { "diratime", 0, 1, MS_NODIRATIME },  /* Update dir access times */
  { "nodiratime", 0, 0, MS_NODIRATIME },/* Do not update dir access times */
#endif
  { NULL,	0, 0, 0		}
};

/* Try to build a canonical options string.  */
static char * fix_opts_string (int flags, const char *extra_opts) {
	const struct opt_map *om;
	char *new_opts;

	new_opts = xstrdup((flags & MS_RDONLY) ? "ro" : "rw");
	if (flags & MS_USER) {
		struct passwd *pw = getpwuid(getuid());
		if(pw)
			new_opts = xstrconcat3(new_opts, ",user=", pw->pw_name);
	}
	
	for (om = opt_map; om->opt != NULL; om++) {
		if (om->skip)
			continue;
		if (om->inv || !om->mask || (flags & om->mask) != om->mask)
			continue;
		new_opts = xstrconcat3(new_opts, ",", om->opt);
		flags &= ~om->mask;
	}
	if (extra_opts && *extra_opts) {
		new_opts = xstrconcat3(new_opts, ",", extra_opts);
	}

	return new_opts;
}

static inline void dup_mntent(struct mntent *ment, nfs_mntent_t *nment)
{
	/* Not sure why nfs_mntent_t should exist */
	nment->mnt_fsname = strdup(ment->mnt_fsname);
	nment->mnt_dir = strdup(ment->mnt_dir);
	nment->mnt_type = strdup(ment->mnt_type);
	nment->mnt_opts = strdup(ment->mnt_opts);
	nment->mnt_freq = ment->mnt_freq;
	nment->mnt_passno = ment->mnt_passno;
}
static inline void 
free_mntent(nfs_mntent_t *ment, int remount)
{
	free(ment->mnt_fsname);
	free(ment->mnt_dir);
	free(ment->mnt_type);
	/* 
	 * Note: free(ment->mnt_opts) happens in discard_mntentchn()
	 * via update_mtab() on remouts
	 */
	 if (!remount)
	 	free(ment->mnt_opts);
}

int add_mtab(char *fsname, char *mount_point, char *fstype, int flags, char *opts, int freq, int passno)
{
	struct mntent ment;
	FILE *mtab;
	int res = 1;

	ment.mnt_fsname = fsname;
	ment.mnt_dir = mount_point;
	ment.mnt_type = fstype;
	ment.mnt_opts = fix_opts_string(flags, opts);
	ment.mnt_freq = freq;
	ment.mnt_passno= passno;

	if(flags & MS_REMOUNT) {
		nfs_mntent_t nment;
		
		dup_mntent(&ment, &nment);
		update_mtab(nment.mnt_dir, &nment);
		free_mntent(&nment, 1);
		return 0;
	}

	lock_mtab();

        if ((mtab = setmntent(MOUNTED, "a+")) == NULL) {
		fprintf(stderr, "Can't open " MOUNTED);
		goto end;
	}

        if (addmntent(mtab, &ment) == 1) {
		fprintf(stderr, "Can't write mount entry");
		goto end;
	}

	endmntent(mtab);
	res = 0;
end:
	unlock_mtab();
	return res;
}

int do_mount_syscall(char *spec, char *node, char *type, int flags, void *data)
{
	return mount(spec, node, type, flags, data);
}

void mount_usage()
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

static inline void
parse_opt(const char *opt, int *mask, char *extra_opts, int len) {
	const struct opt_map *om;

	for (om = opt_map; om->opt != NULL; om++) {
		if (!strcmp (opt, om->opt)) {
			if (om->inv)
				*mask &= ~om->mask;
			else
				*mask |= om->mask;
			return;
		}
	}

	len -= strlen(extra_opts);

	if (*extra_opts && --len > 0)
		strcat(extra_opts, ",");

	if ((len -= strlen(opt)) > 0)
		strcat(extra_opts, opt);
}

/* Take -o options list and compute 4th and 5th args to mount(2).  flags
   gets the standard options (indicated by bits) and extra_opts all the rest */
static void parse_opts (const char *options, int *flags, char **extra_opts)
{
	if (options != NULL) {
		char *opts = xstrdup(options);
		char *opt, *p;
		int len = strlen(opts) + 256;
		int open_quote = 0;
 
		*extra_opts = xmalloc(len);
		**extra_opts = '\0';

		for (p=opts, opt=NULL; p && *p; p++) {
			if (!opt)
				opt = p;		/* begin of the option item */
			if (*p == '"') 
				open_quote ^= 1;	/* reverse the status */
			if (open_quote)
				continue;		/* still in quoted block */
			if (*p == ',')
				*p = '\0';		/* terminate the option item */
			/* end of option item or last item */
			if (*p == '\0' || *(p+1) == '\0') {
				parse_opt(opt, flags, *extra_opts, len);
				opt = NULL;
			}
		} 
		free(opts);
	}
}

/*
 * Look for an option in a comma-separated list
 */
int
contains(const char *list, const char *s) {
	int n = strlen(s);

	while (*list) {
		if (strncmp(list, s, n) == 0 &&
		  (list[n] == 0 || list[n] == ','))
			return 1;
		while (*list && *list++ != ',') ;
	}
	return 0;
}

/*
 * If list contains "user=peter" and we ask for "user=", return "peter"
 */
char *
get_value(const char *list, const char *s) {
	const char *t;
	int n = strlen(s);

	while (*list) {
		if (strncmp(list, s, n) == 0) {
			s = t = list+n;
			while (*s && *s != ',')
				s++;
			return xstrndup(t, s-t);
		}
		while (*list && *list++ != ',') ;
	}
	return 0;
}

static void mount_error(char *mntpnt, char *node)
{
	switch(errno) {
		case ENOTDIR:
			fprintf(stderr, "%s: mount point %s is not a directory\n", 
				progname, mntpnt);
			break;
		case EBUSY:
			fprintf(stderr, "%s: %s is already mounted or busy\n", 
				progname, mntpnt);
			break;
		case ENOENT:
			if (node) {
				fprintf(stderr, "%s: %s failed, reason given by server: %s\n",
					progname, node, strerror(errno));
			} else
				fprintf(stderr, "%s: mount point %s does not exist\n", 
					progname, mntpnt);
			break;
		default:
			fprintf(stderr, "%s: %s\n", progname, strerror(errno));
	}
}
static int chk_mountpoint(char *mount_point)
{
	struct stat sb;

	if (stat(mount_point, &sb) < 0){
		mount_error(mount_point, NULL);
		return 1;
	}
	if (S_ISDIR(sb.st_mode) == 0){
		errno = ENOTDIR;
		mount_error(mount_point, NULL);
		return 1;
	}
	if (access(mount_point, X_OK) < 0) {
		mount_error(mount_point, NULL);
		return 1;
	}

	return 0;
}
#define NFS_MOUNT_VERS_DEFAULT 3

int main(int argc, char *argv[])
{
	int c, flags = 0, mnt_err = 1, fake = 0;
	char *spec, *mount_point, *extra_opts = NULL;
	char *mount_opts = NULL, *p;
	struct mntentchn *mc;
	uid_t uid = getuid();

	progname = argv[0];
	if ((p = strrchr(progname, '/')) != NULL)
		progname = p+1;

	if(!strncmp(progname, "umount", strlen("umount"))) {
		if(argc < 2) {
			umount_usage();
			exit(1);
		}
		return(nfsumount(argc, argv));
	}

	if ((argc < 2)) {
		mount_usage();
		exit(1);
	}

	if(argv[1][0] == '-') {
		if(argv[1][1] == 'V')
			printf("%s ("PACKAGE_STRING")\n", progname);
		else
			mount_usage();
		return 0;
	}

	while ((c = getopt_long (argc - 2, argv + 2, "rt:vVwfno:hs",
				longopts, NULL)) != -1) {
		switch (c) {
		case 'r':
			flags |= MS_RDONLY;
			break;
		case 't':
			if (strcmp(optarg, "enfs")) {
				fprintf(stderr, "%s: unable to handle `%s'\n",
					progname, optarg);
				exit(1);
			}
			break;
		case 'v':
			++verbose;
			break;
		case 'V':
			printf("%s: ("PACKAGE_STRING")\n", progname);
			return 0;
		case 'w':
			flags &= ~MS_RDONLY;
			break;
		case 'f':
			++fake;
			break;
		case 'n':
			++nomtab;
			break;
		case 'o':              /* specify mount options */
			if (mount_opts)
				mount_opts = xstrconcat3(mount_opts, ",", optarg);
			else
				mount_opts = xstrdup(optarg);
			break;
		case 's':
			++sloppy;
			break;
		case 128: /* bind */
			mounttype = MS_BIND;
			break;
		case 129: /* replace */
			mounttype = MS_REPLACE;
			break;
		case 130: /* after */
			mounttype = MS_AFTER;
			break;
		case 131: /* before */
			mounttype = MS_BEFORE;
			break;
		case 132: /* over */
			mounttype = MS_OVER;
			break;
		case 133: /* move */
			mounttype = MS_MOVE;
			break;
		case 135: /* rbind */
			mounttype = MS_BIND | MS_REC;
			break;
		case 'h':
		default:
			mount_usage();
			exit(1);
		}
	}

	spec = argv[1];
	mount_point = canonicalize(argv[2]);

	parse_opts(mount_opts, &flags, &extra_opts);

	if (uid != 0 && !(flags & MS_USERS) && !(flags & MS_USER)) {
		fprintf(stderr, "%s: permission denied\n", progname);
		exit(1);
	}

	if ((flags & MS_USER || flags & MS_USERS) && uid != 0) {
		/* check if fstab has entry, and further see if the user or users option is given */
		if ((mc = getfsspec(spec)) == NULL &&
		    (mc = getfsfile(spec)) == NULL) {
			fprintf(stderr, "%s: permission denied - invalid option\n", progname);
			exit(1);
		}
		else {
			if((flags & MS_USER) && !contains(mc->m.mnt_opts, "user")) {
				fprintf(stderr, "%s: permission denied - invalid option\n", progname);
				exit(1);
			}
			if((flags & MS_USERS) && !contains(mc->m.mnt_opts, "users")) {
				fprintf(stderr, "%s: permission denied - invalid option\n", progname);
				exit(1);
			}
		}
	}

	if (chk_mountpoint(mount_point))
		exit(EX_FAIL);

	if (!strcmp(progname, "mount.enfs")) {
		mnt_err = nfsmount(spec, mount_point, &flags,
				&extra_opts, &mount_opts,  0);
	}
	if (mnt_err)
		exit(EX_FAIL);

	if (!fake) {
		mnt_err = do_mount_syscall(spec, mount_point,
				"enfs",
				flags, mount_opts);

		if (mnt_err) {
			mount_error(mount_point, spec);
			exit(EX_FAIL);
		}
	}

	if (!nomtab) {
		add_mtab(spec, mount_point, "enfs",
			 flags, extra_opts, 0, 0);
	}

	return 0;
}

