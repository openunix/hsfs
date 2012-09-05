#ifndef __API_H__
#define __API_H__

/*
 * Usage: mapping a path on a Unix NFS server to its FH.
 * @svr_ip[in]: indicating the IP or hostname of the NFS server, MUST be valid.
 * @path[in]: indicating the path (in utf8, unix format), MUST be valid.
 * @fhlen[out]: the length of NFSv3 FH.
 * @fhvalp[out]: pointing to a buffer in which content of FH stored. $fhlen can
 * 	be used for indicating how many valid bytes in $fhvalp.
 *
 * Return Value: 0 for succ, others errno <errno.h>
 *
 * Notes:
 * 1. $fhlen and $fhvalp MUST be valid!
 * 2. If /foo/bar/ is the exported dir of NFS Server, path should be a sub-path
 * 	under /foor/bar/
 */
extern int map_path_to_nfs3fh(const char *svr_ip, const char *path,
	size_t *fhlen, unsigned char **fhvalp);

#endif /* __API_H__ */
