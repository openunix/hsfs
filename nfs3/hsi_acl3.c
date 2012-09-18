#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "hsfs.h"
#include "acl.h"

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

/*
 *Convert from in-memory to acl
 *
 */

struct posix_acl *
posix_acl_from_xattr(const void *value, size_t size)
{
	posix_acl_xattr_header *header = (posix_acl_xattr_header *)value;
	posix_acl_xattr_entry *entry = (posix_acl_xattr_entry *)(header+1),
*end;
	int count;
	size_t getcount = size;
	struct posix_acl *acl;
	struct posix_acl_entry *acl_e;

	if (!value)
		return NULL;
	if (size < sizeof(posix_acl_xattr_header))
		 return NULL;
	if (header->a_version != POSIX_ACL_XATTR_VERSION)
		return NULL;

	if (getcount < sizeof(posix_acl_xattr_header))
		return NULL;
	getcount -= sizeof(posix_acl_xattr_header);
	if (getcount % sizeof(posix_acl_xattr_entry))
		return NULL;          
	count = getcount / sizeof(posix_acl_xattr_entry);
	if (count < 0)
		return NULL;
	if (count == 0)
		return NULL;
	
	acl = posix_acl_alloc(count);
	if (!acl)
		return NULL;
	acl_e = acl->a_entries;
	
	for (end = entry + count; entry != end; acl_e++, entry++) {
		acl_e->e_tag  = entry->e_tag;
		acl_e->e_perm = entry->e_perm;

		switch(acl_e->e_tag) {
			case ACL_USER_OBJ:
			case ACL_GROUP_OBJ:
			case ACL_MASK:
			case ACL_OTHER:
				acl_e->e_id = ACL_UNDEFINED_ID;
				break;

			case ACL_USER:
			case ACL_GROUP:
				acl_e->e_id = entry->e_id;
				break;

			default:
				goto fail;
		}
	}
	return acl;

fail:
	free(acl);
	return NULL;
}

struct posix_acl *
posix_acl_from_mode(mode_t mode)
{
	struct posix_acl *acl = posix_acl_alloc(3);
	if (!acl)
		return NULL;

	acl->a_entries[0].e_tag  = ACL_USER_OBJ;
	acl->a_entries[0].e_id   = ACL_UNDEFINED_ID;
	acl->a_entries[0].e_perm = (mode & S_IRWXU) >> 6;

	acl->a_entries[1].e_tag  = ACL_GROUP_OBJ;
	acl->a_entries[1].e_id   = ACL_UNDEFINED_ID;
	acl->a_entries[1].e_perm = (mode & S_IRWXG) >> 3;

	acl->a_entries[2].e_tag  = ACL_OTHER;
	acl->a_entries[2].e_id   = ACL_UNDEFINED_ID;
	acl->a_entries[2].e_perm = (mode & S_IRWXO);
	return acl;
}
