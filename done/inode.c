#include "inode.h"
#include "sector.h"
#include "error.h"
#include "unixv6fs.h"
#include <inttypes.h>

int inode_scan_print(const struct unix_filesystem *u)
{
	M_REQUIRE_NON_NULL(u);
    uint16_t sector = (u->s).s_inode_start; // first sector containing an inode
    uint16_t size = (u->s).s_isize; // number of sectors containing inodes

    /* iteration on the sectors */
    for(uint16_t s = 0; s < size; ++s) { // s is uint16_t the sector number
        struct inode inodes[INODES_PER_SECTOR];
        int error = sector_read(u->f, sector + s, inodes);

        /* an error occured while trying to read sector */
        if(error) {
            return error;
        }

        /* iteration on the sector's inodes */
        for(int i = 0; i < INODES_PER_SECTOR; ++i) {

            /* print only if inode is valid */
            if(inodes[i].i_mode & IALLOC) {
                uint16_t currentInode = INODES_PER_SECTOR * s + i; // number of the inode in current sector
                
                printf("inode %3u ", currentInode);

                /* check wether inode is a directory or a file */
                if (inodes[i].i_mode & IFDIR) {
                    printf("(%s) ", SHORT_DIR_NAME);
                } else {
                    printf("(%s) ", SHORT_FIL_NAME);
                }

                printf("len %" PRIu32"\n", inode_getsize(&inodes[i])); // call inode_getsize with pointer to current inode
                fflush(stdout);
            }
        }
    }
    return 0;
}

void inode_print(const struct inode *inode)
{
	printf("**********FS INODE START**********\n");
	if(inode == NULL)
	{
		printf("NULL ptr\n");
	}
	else
	{
		printf("i_mode: %u\n",inode->i_mode);
		printf("i_nlink: %u\n",inode->i_nlink);
		printf("i_uid: %u\n",inode->i_uid);
		printf("i_gid: %u\n",inode->i_gid);
		printf("i_size0: %u\n",inode->i_size0);
		printf("i_size1: %u\n",inode->i_size1);
		uint32_t size = inode->i_size0;
		size = size << 16;
		size += inode->i_size1;
		printf("size: %u\n",size);
	}
	printf("**********FS INODE END**********\n");
	fflush(stdout);
}
