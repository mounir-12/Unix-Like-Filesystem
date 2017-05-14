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
    for(uint32_t s = 0; s < size; ++s) { // s the sector number is uint32_t to get correct value passed to sector_read
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
    if(inode == NULL) {
        printf("NULL ptr\n");
    } else {
        printf("i_mode: %u\n",inode->i_mode);
        printf("i_nlink: %u\n",inode->i_nlink);
        printf("i_uid: %u\n",inode->i_uid);
        printf("i_gid: %u\n",inode->i_gid);
        printf("i_size0: %u\n",inode->i_size0);
        printf("i_size1: %u\n",inode->i_size1);
        printf("size: %u\n",inode_getsize(inode));
    }
    printf("**********FS INODE END**********\n");
    fflush(stdout);
}


int inode_read(const struct unix_filesystem *u, uint16_t inr, struct inode *inode)
{
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(inode);

    uint16_t start = (u->s).s_inode_start;	// first sector containing an inode
    uint16_t size = (u->s).s_isize; // number of sectors containing inodes

    uint32_t maxInodeNb = INODES_PER_SECTOR * size - 1; // last valid inode number

    if( !(inr >=0 && inr<=maxInodeNb)) { // if not in the range [0; maxInodeNb]
        return ERR_INODE_OUTOF_RANGE; // return approriate error code
    }

    // read sector
    struct inode inodes[INODES_PER_SECTOR];
    uint32_t sectorNb = (inr - (inr % INODES_PER_SECTOR)) / INODES_PER_SECTOR; // sector number for inode inr

    int error = sector_read(u->f, start + sectorNb, inodes);
    /* an error occured while trying to read sector */
    if(error) {
        return error; // return approriate error code
    }

    int i = inr % INODES_PER_SECTOR; // index of inode inr in inodes array

    if(!(inodes[i].i_mode & IALLOC)) { // IALLOC flag is 0
        return ERR_UNALLOCATED_INODE; // return approriate error code
    }

    // no error
    *inode = inodes[i]; // copy inode content to memory location pointed by inode pointer
    return 0;
}

// returns the sector number of the (file_sec_off)th sector containing file content
//so if file_sec_off = 4, return sector number of 4th sector if 4th sector contains data
int inode_findsector(const struct unix_filesystem *u, const struct inode *i, int32_t file_sec_off)
{
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(i);

    if(file_sec_off < 0) { // invalid offset
        return ERR_OFFSET_OUT_OF_RANGE; // return approriate error code
    }

    if(!(i->i_mode & IALLOC)) { // IALLOC flag is 0
        return ERR_UNALLOCATED_INODE; // return approriate error code
    }


    uint32_t size = inode_getsize(i); // file size
    uint32_t smallFileMaxSize = (1 << 10) * 4; // small file is 4 * 2^10 bytes = 4 Kbytes
    uint32_t largeFileMaxSize = (1 << 10) * 896; // large file is 896 * 2^10 bytes = 896 Kbytes

    if(size <= smallFileMaxSize) { // small file
        if(file_sec_off * SECTOR_SIZE >= size) { // invalid offset
            return ERR_OFFSET_OUT_OF_RANGE; // return approriate error code
        }
        return i->i_addr[file_sec_off];
    } else if(size <= largeFileMaxSize) { // large file
        if(file_sec_off * SECTOR_SIZE >= size) { // invalid offset
            return ERR_OFFSET_OUT_OF_RANGE; // return approriate error code
        }

        uint16_t sectorOfSectorsNb = i->i_addr[file_sec_off / ADDRESSES_PER_SECTOR]; // number of the sector containing the indirect sectors numbers

        uint16_t sectors[ADDRESSES_PER_SECTOR];
        int error = sector_read(u->f,sectorOfSectorsNb,sectors);

        if(error) { // error occured
            return error;
        } else {
            return sectors[file_sec_off % ADDRESSES_PER_SECTOR]; // return the sector number
        }

    } else { // extra large file
        return ERR_FILE_TOO_LARGE;
    }
}

int inode_write(struct unix_filesystem *u, uint16_t inr, const struct inode *inode)
{
	M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(inode);
    
    uint16_t start = (u->s).s_inode_start;	// first sector containing an inode
    uint16_t size = (u->s).s_isize; // number of sectors containing inodes

    uint32_t maxInodeNb = INODES_PER_SECTOR * size - 1; // last valid inode number

    if( !(inr >=0 && inr<=maxInodeNb)) { // if not in the range [0; maxInodeNb]
        return ERR_INODE_OUTOF_RANGE; // return approriate error code
    }
    
    // read sector
    struct inode inodes[INODES_PER_SECTOR];
    uint32_t sectorNb = (inr - (inr % INODES_PER_SECTOR)) / INODES_PER_SECTOR; // sector number for inode inr

    int readError = sector_read(u->f, start + sectorNb, inodes);
    
    if(readError) {// an error occured while trying to read sector
        return readError; // return approriate error code
    }
    
    int i = inr % INODES_PER_SECTOR; // index of inode inr in inodes array

    if(!(inodes[i].i_mode & IALLOC)) { // IALLOC flag is 0
        return ERR_UNALLOCATED_INODE; // return approriate error code
    }
	
    inodes[i] = *inode; //write the inode in the array
	
    int writeError = sector_write(u->f, start + sectorNb, inodes); //write the modified array to appropriate sector
    
    return writeError;
}


