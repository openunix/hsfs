#ifndef _NFS_FS_I
#define _NFS_FS_I

#include <asm/types.h>
#include <linux/list.h>
/* qinping mod 20060331 begin */
//#include <linux/nfs.h>
#include "nfs.h"
/* qinping mod 20060331 end */

struct nlm_lockowner;

/*
 * NFS lock info
 */
struct nfs_lock_info {
	u32		state;
	u32		flags;
	struct nlm_lockowner *owner;
};

/*
 * Lock flag values
 */
#define NFS_LCK_GRANTED		0x0001		/* lock has been granted */
#define NFS_LCK_RECLAIM		0x0002		/* lock marked for reclaiming */

#endif
