#ifndef __ACL_H
#define __ACL_H

/* Extended attribute names */
#define POSIX_ACL_XATTR_ACCESS	"system.posix_acl_access"
#define POSIX_ACL_XATTR_DEFAULT	"system.posix_acl_default"

/* Supported ACL a_version fields */
#define POSIX_ACL_XATTR_VERSION	0x0002

/* An undefined entry e_id value */
#define ACL_UNDEFINED_ID	(-1)

/* a_type field in acl_user_posix_entry_t */
#define ACL_TYPE_ACCESS		(0x8000)
#define ACL_TYPE_DEFAULT	(0x4000)
/* e_tag entry in struct posix_acl_entry */
#define ACL_USER_OBJ		(0x01)
#define ACL_USER		(0x02)
#define ACL_GROUP_OBJ		(0x04)
#define ACL_GROUP		(0x08)
#define ACL_MASK		(0x10)
#define ACL_OTHER		(0x20)

/* flags for getacl/setacl mode */
#define NFS_ACL			0x0001
#define NFS_ACLCNT		0x0002
#define NFS_DFACL		0x0004
#define NFS_DFACLCNT		0x0008

/* permissions in the e_perm field */
#define ACL_READ		(0x04)
#define ACL_WRITE		(0x02)
#define ACL_EXECUTE		(0x01)
//#define ACL_ADD		(0x08)



/* A define for big endian and little endian we just use the type define  */
typedef unsigned short __le16;
typedef unsigned short __u16;

typedef unsigned int  __le32;
typedef unsigned int  __u32;
#if 0
#ifdef __CHECKER__
#define __bitwise__ __attribute__((bitwise))
#else
#define __bitwise__
#endif
#ifdef __CHECK_ENDIAN__
#define __bitwise __bitwise__
#else
#define __bitwise
#endif
#endif
typedef struct {
	volatile int counter;
} atomic_t;

struct posix_acl_entry {
	short e_tag;
	unsigned short	e_perm;
	unsigned int	e_id;
};
struct posix_acl {
	atomic_t a_refcount;
	unsigned int a_count;
	struct posix_acl_entry	a_entries[0];
};

typedef struct {
	__le16	e_tag;
	__le16	e_perm;
	__le32	e_id;
} posix_acl_xattr_entry;

typedef struct {
	__le32	a_version;
	posix_acl_xattr_entry	a_entries[0];
} posix_acl_xattr_header;

static inline size_t posix_acl_xattr_size (int count)
{
	return (sizeof(posix_acl_xattr_header) +
		(count * sizeof(posix_acl_xattr_entry)));
}

extern struct posix_acl *posix_acl_alloc(int count);
extern int posix_acl_to_xattr(const struct posix_acl *acl, void *buffer, 
				size_t size);
extern struct posix_acl *posix_acl_from_xattr(const void *value, size_t size);
struct posix_acl *posix_acl_from_mode(mode_t mode);
#endif /* */
