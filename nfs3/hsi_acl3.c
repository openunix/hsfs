#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "acl.h"
#include "log.h"

#define atomic_set(v,i)		((v)->counter = (i))
/*
 *  * Allocate a new ACL with the specified number of entries.
 *   */
struct posix_acl *posix_acl_alloc(int count)
{
	const size_t size = sizeof(struct posix_acl) +
		                    count * sizeof(struct posix_acl_entry);
 	struct posix_acl *acl = (struct posix_acl *)calloc(1,size);
	if (acl) {
			atomic_set(&acl->a_refcount, 1);
			acl->a_count = count;
		}
	return acl;
}

/*
 *  * Convert from in-memory to extended attribute representation.
 *   */
int posix_acl_to_xattr(const struct posix_acl *acl, void *buffer, size_t size)
{
	posix_acl_xattr_header *ext_acl = NULL;
	posix_acl_xattr_entry *ext_entry = NULL;
	int real_size, n;

	real_size = posix_acl_xattr_size(acl->a_count);
	if (!buffer)
		return real_size;
	if (real_size > size)
		return ERANGE;
	ext_acl = (posix_acl_xattr_header *)buffer;
	ext_entry = ext_acl->a_entries;
	
	ext_acl->a_version = POSIX_ACL_XATTR_VERSION;

	for (n=0; n < acl->a_count; n++, ext_entry++) {
		ext_entry->e_tag  = acl->a_entries[n].e_tag;
		ext_entry->e_perm = acl->a_entries[n].e_perm;
		ext_entry->e_id   = acl->a_entries[n].e_id;
	}
	return real_size;
}

