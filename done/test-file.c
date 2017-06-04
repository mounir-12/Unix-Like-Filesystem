#include <string.h>
#include "mount.h"
#include "inode.h"
#include "error.h"
#include "unixv6fs.h"
#include "sector.h"
#include "filev6.h"
#include "sha.h"

void print_inode(const struct unix_filesystem *u, uint16_t inr);
void print_all_SHA(struct unix_filesystem *u);

void test(struct unix_filesystem *u)
{
    struct filev6 fs;
    memset(&fs, 255, sizeof(fs));
    print_inode(u,3);
    print_inode(u,5);
    print_all_SHA(u);

}

void print_inode(const struct unix_filesystem *u, uint16_t inr)
{
    struct inode n;
    int error = inode_read(u, inr, &n);
    if(error) {
        printf("filev6_open failed for inode #%u.\n\n", inr);
    } else {
        printf("Printing inode #%u:\n", inr);
        inode_print(&n);
        if ((n.i_mode & IFDIR)) {
            printf("which is a directory.\n\n");
        } else {
            printf("the first sector of data of which contains:\n");
            char firstSector[SECTOR_SIZE+1];
            firstSector[SECTOR_SIZE] = '\0';
            sector_read(u->f,inode_findsector(u,&n,0),firstSector);
            printf("%s\n",firstSector);
            printf("----\n\n");
        }
    }
}

void print_all_SHA(struct unix_filesystem *u)
{
    if(u == NULL) {
        return;
    }
    uint16_t sector = (u->s).s_inode_start; // first sector containing an inode
    uint16_t size = (u->s).s_isize; // number of sectors containing inodes

    printf("Listing inodes SHA:\n");
    /* iteration on the sectors */
    for(uint32_t s = 0; s < size; ++s) { // s the sector number is uint32_t to get correct value passed to sector_read
        struct inode inodes[INODES_PER_SECTOR];
        int error = sector_read(u->f, sector + s, inodes);

        /* an error occured while trying to read sector */
        if(error) {
            return;
        }

        /* iteration on the sector's inodes */
        for(int i = 0; i < INODES_PER_SECTOR; ++i) {
            uint16_t currentInode = INODES_PER_SECTOR * s + i; // number of the inode in current sector
            print_sha_inode(u,inodes[i],currentInode);
        }
    }
}
