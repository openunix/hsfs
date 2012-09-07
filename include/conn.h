/* 
 * conn.h -- Connection routines for NFS mount / umount code.
 *
 * 2006-06-06 Amit Gud <agud@redhat.com>
 * - Moved code snippets here from util-linux/mount
 */

#ifndef _CONN_H
#define _CONN_H

#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpc/clnt.h>

#define MNT_SENDBUFSIZE ((u_int)2048)
#define MNT_RECVBUFSIZE ((u_int)1024)

typedef struct {
	char **hostname;
	struct sockaddr_in saddr;
	struct pmap pmap;
} clnt_addr_t;

/* RPC call timeout values */
static const struct timeval TIMEOUT = { 20, 0 };
static const struct timeval RETRY_TIMEOUT = { 3, 0 };

#endif /* _CONN_H */

