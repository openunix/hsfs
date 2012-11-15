#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "hsfs.h"
#include "hsi_nfs3.h"
#include "log.h"

#define  HSFS_TABLE_SIZE  4096

/**
 * hsfs_ino_hash:	hash function
 *
 * @param sb[IN]:	the information of the superblock
 *  
 * @param ino[IN]:	the inode number
 *
 * @return:	the hash value
 *
 * */
static uint64_t hsfs_ino_hash(struct hsfs_super *sb, uint64_t ino)
{
	uint64_t hash = ino % sb->hsfs_fuse_ht.size;
	return hash;
}

/**
 * hsx_fuse_idel:	Remove a node from the Hash Table
 *
 * @param sb[IN]:	the information of the superblock
 *  
 * @param hsfs_node[IN]:a pointer to an object of type hash_inode which 
 * describe the node information in memory
 *
 * @return:	0 if successfull, other values show an error
 *
 * */

int  hsx_fuse_idel(struct hsfs_super *sb,struct hsfs_inode *hs_node)
{
	DEBUG("Enter hsx_fuse_idel()");
	if(sb == NULL || hs_node == NULL)
	{
		DEBUG("Delete failed : the input parameter is invalid !");
		return EINVAL;
	}
	uint64_t hash=hsfs_ino_hash(sb,hs_node->ino);

	struct hsfs_inode **hs_nodep = &sb->hsfs_fuse_ht.array[hash];
	for (; *hs_nodep != NULL; hs_nodep = &(*hs_nodep)->next)
		if (*hs_nodep == hs_node)
		{
			*hs_nodep = hs_node->next;
			sb->hsfs_fuse_ht.use--;
			free(hs_node->fh.data.data_val);
			free(hs_node);
			DEBUG("Exit hsx_fuse_idel : delete success ");
			return  0;
		}
		
	return EINVAL;
}


/**
 * hsx_fuse_itable_del:	Remove and free all member from the hashtable
 *
 * @param sb[IN]:	the information of the superblock
 *  
 * @return:	0 if successfull, other values show an error
 *
 * */
int  hsx_fuse_itable_del(struct hsfs_super *sb)
{
	struct hsfs_inode *hsfs_node;
	struct hsfs_inode *hsfs_inext;
	int hashval;

	if(sb == NULL)
		return EINVAL;
	if(sb->hsfs_fuse_ht.array == NULL)
		return 0;
	for(hashval = 0; hashval < sb->hsfs_fuse_ht.size; hashval++)
	{
		hsfs_node = sb->hsfs_fuse_ht.array[hashval];
		while (hsfs_node)
		{
			hsfs_inext = hsfs_node->next;
			free(hsfs_node->fh.data.data_val);
			free(hsfs_node);
			hsfs_node = hsfs_inext;
		}
	}
	free(sb->hsfs_fuse_ht.array);
	sb->hsfs_fuse_ht.array = NULL;
	return 0;
}



