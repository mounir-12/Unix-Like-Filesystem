#include "inode.h"
#include "sector.h"
#include "error.h"
#include "unixv6fs.h"
#include <inttypes.h>

/**
 * @brief read all inodes from disk and print out their content to
 *        stdout according to the assignment
 * @param u the filesystem
 * @return 0 on success; < 0 on error.
 */
int inode_scan_print(const struct unix_filesystem *u)
{
	M_REQUIRE_NON_NULL(u);
    uint16_t sector = (u->s).s_inode_start; // first sector containing an inode
    uint16_t size = (u->s).s_isize; // number of sectors containing inodes

    /* iteration on the sectors */
    for(int s = 0; s < size; ++s) {
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
                int currentInode = INODES_PER_SECTOR * s + i; // number of the first inode in current sector
                
                printf("inode %3d ", currentInode);

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
