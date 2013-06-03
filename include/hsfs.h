
#ifndef __HSFS_H__
#define __HSFS_H__

#include <hsfs/types.h>
#include <assert.h>

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif

#include <fuse/fuse_lowlevel.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "nfs3.h"
#include "log.h"

#include "hsfs/hashtable.h"

#define HSFS_TYPE "hsfs"

/* FUSE version <= 2.7 don't have this */
#ifndef FUSE_SET_ATTR_ATIME_NOW
# define FUSE_SET_ATTR_ATIME_NOW	(1 << 7)
#endif	/* FUSE_SET_ATTR_ATIME_NOW */

#ifndef FUSE_SET_ATTR_MTIME_NOW
# define FUSE_SET_ATTR_MTIME_NOW	(1 << 8)
#endif /* FUSE_SET_ATTR_MTIME_NOW */

#define S_ISSETMODE(to_set)       ((to_set) & HSFS_ATTR_MODE)
#define S_ISSETUID(to_set)        ((to_set) & HSFS_ATTR_UID)
#define S_ISSETGID(to_set)        ((to_set) & HSFS_ATTR_GID)
#define S_ISSETSIZE(to_set)       ((to_set) & HSFS_ATTR_SIZE)
#define S_ISSETATIME(to_set)      ((to_set) & HSFS_ATTR_ATIME_SET)
#define S_ISSETMTIME(to_set)      ((to_set) & HSFS_ATTR_MTIME_SET)
#define S_ISSETATIMENOW(to_set)   ((to_set) & HSFS_ATTR_ATIME)
#define S_ISSETMTIMENOW(to_set)   ((to_set) & HSFS_ATTR_MTIME)

#define S_SETMODE(sattr)          ((sattr)->valid |= HSFS_ATTR_MODE)
#define S_SETUID(sattr)           ((sattr)->valid |= HSFS_ATTR_UID)
#define S_SETGID(sattr)           ((sattr)->valid |= HSFS_ATTR_GID)
#define S_SETSIZE(sattr)          ((sattr)->valid |= HSFS_ATTR_SIZE)
#define S_SETATIME(sattr)         ((sattr)->valid |= HSFS_ATTR_ATIME)
#define S_SETATIMENOW(sattr)      ((sattr)->valid |= HSFS_ATTR_ATIME_NOW)
#define S_SETMTIME(sattr)         ((sattr)->valid |= HSFS_ATTR_MTIME)
#define S_SETMTIMENOW(sattr)      ((sattr)->valid |= HSFS_ATTR_MTIME_NOW)


extern char *progname;
extern int nomtab;
extern int verbose;
extern int fg;

extern int hsfs_init();

/* Macros or fake-macros.... */
extern unsigned int HSFS_PAGE_SIZE;

/*
 * To change the maximum rsize and wsize supported by the NFS client, adjust
 * NFS_MAX_FILE_IO_SIZE.  64KB is a typical maximum, but some servers can
 * support a megabyte or more.  The default is left at 4096 bytes, which is
 * reasonable for NFS over UDP.
 */
#define HSFS_MAX_FILE_IO_SIZE	(1048576U)
#define HSFS_DEF_FILE_IO_SIZE	(4096U)
#define HSFS_MIN_FILE_IO_SIZE	(1024U)

/*
 * Maximum number of pages that readdir can use for creating
 * a vmapped array of pages.
 */
#define HSFS_MAX_READDIR_PAGES 8

struct hsfs_iattr{
	unsigned int valid;	/* See below */
	mode_t           mode;
	uid_t	         uid;
	gid_t	         gid;
	off_t	         size;
	struct timespec  atime;
	struct timespec  mtime;
	struct timespec ctime;
};

/* For hsfs_iattr.valid */
#define HSFS_ATTR_MODE	(1 << 0)
#define HSFS_ATTR_UID	(1 << 1)
#define HSFS_ATTR_GID	(1 << 2)
#define HSFS_ATTR_SIZE	(1 << 3)
#define HSFS_ATTR_ATIME (1 << 4)
#define HSFS_ATTR_MTIME (1 << 5)
#define HSFS_ATTR_CTIME (1 << 6)
#define HSFS_ATTR_ATIME_SET (1 << 7)
#define HSFS_ATTR_MTIME_SET (1 << 8)


struct hsfs_inode
{
	struct hsfs_super *sb;
	struct hsfs_iattr iattr;
	unsigned int i_state;
	unsigned int i_nlink;
	uint64_t i_blocks;
	struct hlist_node fh_hash;
	struct hlist_node id_hash;
	unsigned long private;
	int i_count;
	unsigned long real_ino;
	unsigned int i_blkbits;
	uint64_t          ino;
	dev_t i_rdev;
	unsigned long     generation;
  
};

/* All the i_ except i_state is actually in hsfs_iattr */
#define i_mode iattr.mode
#define i_atime iattr.atime
#define i_mtime iattr.mtime
#define i_ctime iattr.ctime
#define i_size iattr.size
#define i_uid iattr.uid
#define i_gid iattr.gid

static inline off_t i_size_read(const struct hsfs_inode *inode)
{
	return (inode->i_size);
}

#define I_NEW (1UL << 3)

/**
 *is_bad_inode - is an inode errored
 *@inode: inode to test
 *
 *Returns true if the inode in question has been marked as bad.
 */
 
static inline int is_bad_inode(struct hsfs_inode *inode)
{
	return (inode->ino == 0);
}

struct  hsfs_table
{
  struct hsfs_inode  **array;
  size_t  use;
  size_t  size;
};

#define HSFS_FH_HASH_BITS 10
#define HSFS_FH_HASH_SIZE (1 << HSFS_NAME_HASH_BITS)
#define HSFS_ID_HASH_BITS 10
#define HSFS_ID_HASH_SIZE (1 << HSFS_ID_HASH_BITS)

struct nfs_fattr;
struct hsfs_super_ops
{
	struct hsfs_inode *(*alloc_inode)(struct hsfs_super *sb);
	void (*destroy_inode)(struct hsfs_inode *);
	void (*drop_inode) (struct hsfs_inode *);
	int (*setattr)(struct hsfs_inode *, struct nfs_fattr *, struct hsfs_iattr *);
};

struct hsfs_super
{
	DECLARE_HASHTABLE(fh_table, HSFS_FH_HASH_BITS); /* For HSFS iget5 */
	DECLARE_HASHTABLE(id_table, HSFS_ID_HASH_BITS);	/* For HSFS iget/ilookup */
	struct hsfs_super_ops *sop;

	/* XXX need protected by mutex */
	unsigned long curr_id;

	/* XXX Should put them into nfs_super */
  CLIENT *clntp;
  CLIENT *acl_clntp;
  int    flags;
  /* for read/write */
  unsigned int    rsize;
  unsigned int	 wsize;
  /* For all clnt_call timeout,
   * as deciseconds (tenths of a second)
   */
  int	 timeo;
  int    retrans;
  int	 acregmin;
  int	 acregmax;
  int	 acdirmin;
  int	 acdirmax;
  struct sockaddr_in	addr;
  /* for statfs */
  int	 namlen;
  /* Readdir size */
  unsigned int	 dtsize;
  unsigned int	    bsize;
  unsigned char	    bsize_bits;
  struct hsfs_inode *root;
  /* Copy from FSINFO result directly, be careful */
  uint32_t	    rtmax;
  uint32_t	    rtpref;
  uint32_t	    rtmult;
  uint32_t	    wtmax;
  uint32_t	    wtpref;
  uint32_t	    wtmult;
  uint32_t	    dtpref;
  uint64_t	    maxfilesize;
  nfstime3	    time_delta;
  uint32_t	    properties;
  
  /* Copy from FSSTAT result directly, be careful */
  uint64_t	    tbytes;
  uint64_t	    fbytes;
  uint64_t	    abytes;
  uint64_t	    tfiles;
  uint64_t	    ffiles;
  uint64_t	    afiles;
};

#define HSFS_FILE_SYNC  2
#define HSFS_UNSTABLE	0

struct hsfs_rw_info {
  struct hsfs_inode	*inode;
  size_t	  	rw_size;//read or write size
  off_t			rw_off;//read or write offset
  size_t		ret_count;//result of operate
  int			eof;//end of file
  int			stable;//direct io flag
  struct {
    size_t data_len;
    char *data_val;
  } data;//data buffer
};


struct hsfs_readdir_ctx{
	off_t		off;
	char		*name;
	char		cookieverf[NFS3_COOKIEVERFSIZE];
	struct stat	stbuf;
	struct hsfs_readdir_ctx *next;
	struct hsfs_inode *inode;
};

/* for nfs3 */
unsigned long hsfs_block_bits(unsigned long bsize, unsigned char *nrbitsp);
unsigned long hsfs_block_size(unsigned long bsize, unsigned char *nrbitsp);

#define min(x, y) ((x) < (y) ? (x) : (y))
extern int hsfs_init_icache(struct hsfs_super *sb);

/**
 * @brief Lookup the inode cache with ino
 *
 * @param sb[IN] the hsfs superblock
 * @param ino[IN] the hsfs number (non-persistent)
 * @return the pointer to the hsfs inode found if success, else NULL 
 **/
extern struct hsfs_inode *hsfs_ilookup(struct hsfs_super *sb, uint64_t ino);


/**
 * iget5_locked - obtain an inode from a mounted file system
 * @sb:super block of file system
 * @hashval:hash value (usually inode number) to get
 * @test:callback used for comparisons between inodes
 * @set:callback used to initialize a new struct inode
 * @data:opaque data pointer to pass to @test and @set
 *
 * iget5_locked() uses ifind() to search for the inode specified by @hashval
 * and @data in the inode cache and if present it is returned with an increased
 * reference count. This is a generalized version of iget_locked() for file
 * systems where the inode number is not sufficient for unique identification
 * of an inode.
 *
 * If the inode is not in cache, get_new_inode() is called to allocate a new
 * inode and this is returned locked, hashed, and with the I_NEW flag set. The
 * file system gets to fill it in before unlocking it via unlock_new_inode().
 *
 * Note both @test and @set are called with the inode_lock held, so can't sleep.
 */
struct hsfs_inode *hsfs_iget5_locked(struct hsfs_super *sb, unsigned long hashval,
				     int (*test)(struct hsfs_inode *, void *),
				     int (*set)(struct hsfs_inode *, void *), void *data);
/**
 *iput- put an inode
 *@inode: inode to put
 *
 *Puts an inode, dropping its usage count. If the inode use count hits
 *zero, the inode is then freed and may also be destroyed.
 *
 *Consequently, iput() can sleep.
 */
void hsfs_iput(struct hsfs_inode *inode);
void hsfs_unlock_new_inode(struct hsfs_inode *inode);
void hsfs_generic_fillattr(struct hsfs_inode *, struct stat *);
int hsfs_ll_setattr(struct hsfs_inode *inode, struct hsfs_iattr *sattr); 
#endif
