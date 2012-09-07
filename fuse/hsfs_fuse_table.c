#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "hsi_nfs3.h"
#include "log.h"

#define  HSFS_TABLE_SIZE  4096

/**
 * hsx_fuse_itable_init:	initialize the Hash Table
 *
 * @param sb[IN]:	the information of the superblock
 * 
 * @return:	0 if successfull, other values show an error
 *
 * */
int  hsx_fuse_itable_init(struct hsfs_super *sb)
{
	sb->hsfs_fuse_ht.size =  HSFS_TABLE_SIZE ;
	sb->hsfs_fuse_ht.array = (struct hsfs_inode **) calloc(1, sizeof(struct hsfs_inode *) * sb->hsfs_fuse_ht.size);
	if (sb->hsfs_fuse_ht.array == NULL) {
		ERR("hsfs: memory allocation failed.");
		return ENOMEM ;
	}
	sb->hsfs_fuse_ht.use = 0;

	return 0;

}

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
uint64_t hsfs_ino_hash(struct hsfs_super *sb, uint64_t ino)
{
	uint64_t hash = ino % sb->hsfs_fuse_ht.size;
	return hash;
}

/**
 * hsi_nfs3_alloc_node: allocate memory for a node 
 *
 * @param sb[IN]:       the information of the superblock
 * @param pfh[IN]:      a pointer to the filehandle of node 
 * @param ino[IN]:      the inode number
 * @param pattr[IN]:    a pointer to the attributes of node 
 * @return:		a pointer to the object if successfull,else NULL
 *
 * */
struct hsfs_inode *hsi_nfs3_alloc_node(struct hsfs_super *sb, nfs_fh3 *pfh, uint64_t ino, fattr3 *pattr)
{
        struct hsfs_inode *hsfs_node;

        hsfs_node = (struct hsfs_inode *) calloc(1, sizeof(struct hsfs_inode));
        if(hsfs_node == NULL)
        {
                return  NULL;
        }
        if(sb == NULL || pfh == NULL)
        {
                free(hsfs_node);
                return NULL;
        }

        hsfs_node->ino = ino;
        hsfs_node->generation = 1;
        hsfs_node->fh = *pfh;
        hsfs_node->nlookup = 0;
        hsfs_node->sb = sb;
        if(pattr != NULL)
                hsfs_node->attr = *pattr;

        return hsfs_node;
}

/**
 * hsx_fuse_iadd:	set a inode into the Hash Table
 *
 * @param sb[IN]:	the information of the superblock
 *  
 * @param hsfs_node[IN]:a pointer to an object of type hash_inode which 
 * describe the node information in memory
 *
 * @return:	void
 *
 * */
void   hsx_fuse_iadd(struct hsfs_super *sb, struct hsfs_inode *hsfs_node)
{
	uint64_t  hash =  hsfs_ino_hash(sb,hsfs_node->ino);
	hsfs_node->next = sb->hsfs_fuse_ht.array[hash];
	sb->hsfs_fuse_ht.array[hash]  = hsfs_node;
	sb->hsfs_fuse_ht.use++;
}

/**
 * hsx_fuse_iget:	Try to retrieve the node information 
 *
 * @param sb[IN]:	the information of the superblock
 *  
 * @param ino[IN]:	the inode number
 *
 * @return:	a pointer to the object found if successfull,else NULL 
 *
 * */
struct hsfs_inode *hsx_fuse_iget(struct hsfs_super *sb, uint64_t ino)
{
	struct hsfs_inode  *hsfs_node = NULL;
	if(sb == NULL)
		return NULL;
	uint64_t hash =  hsfs_ino_hash(sb,ino);

	for(hsfs_node=sb->hsfs_fuse_ht.array[hash];hsfs_node != NULL;
	    hsfs_node=hsfs_node->next)
		if(hsfs_node->ino  ==  ino)
		{
			hsfs_node->sb = sb;
			return hsfs_node;
		}
			
	return NULL;
}

/**
 * hsi_nfs3_ifind:	Look up a node from hash table  
 *
 * @param sb[IN]:	The information of the superblock
 * @param pfh[IN]:	A pointer to node filehandle 
 * @param pattr[IN]:	A pointer to node attributes
 * @return:	A pointer to the node information
 *
 * */
struct hsfs_inode *hsi_nfs3_ifind(struct hsfs_super *sb, nfs_fh3 *pfh, fattr3 *pattr)
{
	struct hsfs_inode *hsfs_node = NULL;
	if(sb == NULL || pfh == NULL || pattr == NULL)
	{
		return  NULL;
	}
	pattr->fileid |= (1UL<<63);
	uint64_t ino = pattr->fileid;

	hsfs_node = hsx_fuse_iget(sb,ino);
	if (hsfs_node == NULL)
	{
		DEBUG("Now ,add a inode ,ino : %lu.",ino);
		hsfs_node = (struct hsfs_inode *) calloc(1, sizeof(struct hsfs_inode ));
		if (hsfs_node == NULL)
			return NULL;
		hsfs_node->ino = ino;
		hsfs_node->generation = 1;
		hsfs_node->fh = *pfh;
		hsfs_node->sb = sb;
		hsfs_node->attr = *pattr;
		hsfs_node->attr.fileid &= ~(1UL<<63);
		DEBUG("This is a new node ,and now to add to hash table.");
		hsx_fuse_iadd(sb,hsfs_node);
	}
	hsfs_node->nlookup++;
	return hsfs_node;
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
	if(sb == NULL || hs_node == NULL)
	{
		DEBUG("Delete failed : the input parameter is invalid !");
		return EINVAL;
	}
	uint64_t hash=hsfs_ino_hash(sb,hs_node->ino);

	struct hsfs_inode **hs_nodep = &sb->hsfs_fuse_ht.array[hash];
	for (; *hs_nodep != NULL; hs_nodep = &(*hs_nodep)->next)
		if (*hs_nodep == hs_node) {
			*hs_nodep = hs_node->next;
			sb->hsfs_fuse_ht.use--;
			free(hs_node);
			DEBUG("delete success ....");
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
                        free(hsfs_node);
                        hsfs_node = hsfs_inext;
                }

        }
	free(sb->hsfs_fuse_ht.array);
	sb->hsfs_fuse_ht.array = NULL;
        return 0;
}



