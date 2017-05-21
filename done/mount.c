#define DEBUG 1

#include "mount.h"
#include "sector.h"
#include "error.h"
#include "bmblock.h"
#include "inode.h"
#include <string.h>
#include <inttypes.h>

void fill_ibm(struct unix_filesystem *u);
void fill_fbm(struct unix_filesystem *u);

int mountv6(const char *filename, struct unix_filesystem *u)
{
    M_REQUIRE_NON_NULL(filename);
    M_REQUIRE_NON_NULL(u);
    //init u
    memset(u, 0, sizeof(*u));


    u->f = fopen(filename,"r+b"); // open file in u->f in binary read and write mode
    if(u->f == NULL) { // open error
        return ERR_IO;
    }
    uint8_t bootBlock[SECTOR_SIZE];
    int error = sector_read(u->f,BOOTBLOCK_SECTOR,bootBlock); // read boot block sector

    if(!error && bootBlock[BOOTBLOCK_MAGIC_NUM_OFFSET] != BOOTBLOCK_MAGIC_NUM) { // no read error but magic num not found
        return ERR_BADBOOTSECTOR;
    } else if(error) { // read error
        return error;
    } else { // correctly mounted
        int error = sector_read(u->f,SUPERBLOCK_SECTOR,&(u->s)); // read and return the returned value: 0 if success, error otherwise
        if(error) { // error occured
            return error; // propagate error
        }

        uint64_t min_ibm = 2; // first inode (ignoring first two since inode 0 is not used and inode 1 is known to be allocated)
        uint64_t max_ibm = (u->s).s_isize * INODES_PER_SECTOR - 1; // last inode
        u->ibm = bm_alloc(min_ibm, max_ibm); // allocate inode sectors bitmaps
        M_REQUIRE_NON_NULL(u->ibm); // require non NULL
        
        uint64_t min_fbm = (u->s).s_block_start + 1; // data sectors start (ignoring first data sector known to be used)
        uint64_t max_fbm = (u->s).s_fsize-1; // data sectors end
        u->fbm = bm_alloc(min_fbm, max_fbm); // allocate data sectors bitmaps
        M_REQUIRE_NON_NULL(u->fbm); // require non NULL

        fill_ibm(u);
        fill_fbm(u);

        return 0;
    }
}

void mountv6_print_superblock(const struct unix_filesystem *u)
{
    if(u == NULL) {
        return;
    }
    printf("**********FS SUPERBLOCK START**********\n");
    printf("%-19s : %" PRIu16 "\n", "s_isize", (u->s).s_isize);
    printf("%-19s : %" PRIu16 "\n", "s_fsize", (u->s).s_fsize);
    printf("%-19s : %" PRIu16 "\n", "s_fbmsize", (u->s).s_fbmsize);
    printf("%-19s : %" PRIu16 "\n", "s_ibmsize", (u->s).s_ibmsize);
    printf("%-19s : %" PRIu16 "\n", "s_inode_start", (u->s).s_inode_start);
    printf("%-19s : %" PRIu16 "\n", "s_block_start", (u->s).s_block_start);
    printf("%-19s : %" PRIu16 "\n", "s_fbm_start", (u->s).s_fbm_start);
    printf("%-19s : %" PRIu16 "\n", "s_ibm_start", (u->s).s_ibm_start);
    printf("%-19s : %" PRIu8 "\n", "s_flock", (u->s).s_flock);
    printf("%-19s : %" PRIu8 "\n", "s_ilock", (u->s).s_ilock);
    printf("%-19s : %" PRIu8 "\n", "s_fmod", (u->s).s_fmod);
    printf("%-19s : %" PRIu8 "\n", "s_ronly", (u->s).s_ronly);
    printf("%-19s : [%" PRIu16 "] %" PRIu16 "\n", "s_time", (u->s).s_time[0], (u->s).s_time[1]);
    printf("**********FS SUPERBLOCK END**********\n");
    fflush(stdout);

}

int umountv6(struct unix_filesystem *u)
{
    M_REQUIRE_NON_NULL(u);
    if(!fclose(u->f)) { // closed
        return 0;
    } else { // error upon closing
        return ERR_IO;
    }

}

void fill_ibm(struct unix_filesystem *u)
{
    struct bmblock_array *ibm = u->ibm;

    for(uint64_t i = ibm->min; i <= ibm->max; i++) { // iterate over all elements
        bm_clear(ibm, i); // default value
    }

    uint16_t sector = (u->s).s_inode_start; // number of first sector of inodes
    uint16_t size = (u->s).s_isize; // number of sectors containing inodes

    uint64_t current = ibm->min; // current inode

    // iteration on the inode sectors
    for(uint32_t s = 0; s < size; ++s) {
        struct inode inodes[INODES_PER_SECTOR];
        int error = sector_read(u->f, sector + s, inodes);

        // iteration on the sector's inodes
        for(int i = 0; i < INODES_PER_SECTOR; ++i) {
            if(s == 0 && i == 0) { // if read first sector and current inode is inode 0
                i += 2; // skip first two inodes (number 0 and 1)
            }

            // if an error occured while reading the sector, consider
            // all inodes as allocated. Otherwise, check if the inode is
            // allocated.
            if(error || (inodes[i].i_mode & IALLOC)) {
                bm_set(ibm, current);
            }
            current++;
        }
    }
}

void fill_fbm(struct unix_filesystem *u)
{
    struct bmblock_array *fbm = u->fbm;

    for(uint64_t i = fbm->min; i <= fbm->max; i++) { // iterate over all elements
        bm_clear(u->fbm, i); // default value
    }

    uint16_t sector = (u->s).s_inode_start; // number of first sector of inodes
    uint16_t size = (u->s).s_isize; // number of sectors containing inodes

    // iteration on the inode sectors
    for(uint32_t s = 0; s < size; ++s) {
        struct inode inodes[INODES_PER_SECTOR];
        int error = sector_read(u->f, sector + s, inodes);

        if(!error) { // no error occured
            // iteration on the sector's inodes
            for(int i = 0; i < INODES_PER_SECTOR; ++i) {
                if(s == 0 && i == 0) { // if read first sector and current inode is inode 0
                    i += 1; // skip first inode (number 0)
                }

                uint32_t size = inode_getsize(&(inodes[i])); // file size
				uint32_t smallFileMaxSize = ADDR_SMALL_LENGTH * SECTOR_SIZE; // small file is 8 * 512 bytes = 4 Kbytes
				uint32_t largeFileMaxSize = (ADDR_SMALL_LENGTH - 1) * ADDRESSES_PER_SECTOR * SECTOR_SIZE; // large file is 7 * 256 * 512 bytes = 896 Kbytes
				
                if(size > smallFileMaxSize && size <= largeFileMaxSize) { // file is a large file
                    for(int j = 0; j < ADDR_SMALL_LENGTH; j++) { // iterate on indirect sector
                        int sectorNb = inodes[i].i_addr[j]; // get indirect sector numbers
                        if(sectorNb > 0) {
                            bm_set(fbm, sectorNb); // update sector state to be used
                        }
                    }
                }
                int32_t offset = 0; // offset of sector of inode
                int sectorNb = inode_findsector(u, &(inodes[i]), offset); // find sector number of given offset
                while(sectorNb > 0) { // iterate while not last sector used
                    bm_set(fbm, sectorNb); // update sector state to be used
                    offset++; // next offset
                    sectorNb = inode_findsector(u, &(inodes[i]), offset); // find sector number of given offset
                }

            }

        }


    }
}

int mountv6_mkfs(const char *filename, uint16_t num_blocks, uint16_t num_inodes)
{
	M_REQUIRE_NON_NULL(filename);
	
    struct superblock s;
    memset(&s, 0, sizeof(struct superblock));
    
    // size is at least one block in case num_inodes not divisible by INODES_PER_SECTOR
    s.s_isize = num_inodes / INODES_PER_SECTOR; // number of blocks containing inodes
    s.s_fsize = num_blocks; // total number of blocks
    
    if(s.s_fsize < s.s_isize + num_inodes) { // not enough blocks
        return ERR_NOT_ENOUGH_BLOCS; // return appropriate error code
    }
    s.s_inode_start = SUPERBLOCK_SECTOR + 1;
    s.s_block_start = s.s_inode_start + s.s_isize;

    FILE *f = fopen(filename,"w+b"); //open new file
    if(f == NULL) { // open error
        return ERR_IO; // return appropriate error code
    }
    uint8_t bootBlock[SECTOR_SIZE]; //create boot block sector
    bootBlock[BOOTBLOCK_MAGIC_NUM_OFFSET] = BOOTBLOCK_MAGIC_NUM; // set magic number
    int bootBlockError = sector_write(f, BOOTBLOCK_SECTOR, bootBlock); //write boot block sector
    if(bootBlockError) { //error occured while trying to write the boot block sector
		fclose(f); // close file
        return bootBlockError; // propagate error
    }

    int superblockError = sector_write(f, SUPERBLOCK_SECTOR, &s); //write superblock sector
    if(superblockError) { //error occured while trying to write the superblock sector
		fclose(f); // close file
        return superblockError; // propagate error
    }

    for(uint16_t i = s.s_inode_start; i < s.s_block_start ; i++) { // iterate on inodes blocks
        struct inode inodes[INODES_PER_SECTOR];
        memset(inodes, 0, sizeof(struct inode)*INODES_PER_SECTOR); // set all values of the inodes array to zero
        if(i == s.s_inode_start) { // sector containing root
            inodes[ROOT_INUMBER].i_mode = IALLOC | IFDIR;
        }
        int writeError = sector_write(f, i, inodes); //write the modified array to appropriate sector
        if(writeError) {
			fclose(f); // close file
            return writeError; // propagate error
        }
    }
    for(uint16_t i = s.s_block_start; i < s.s_fsize + s.s_inode_start ; i++) { // iterate on data blocks
        char sector[SECTOR_SIZE]; // create sector
        memset(sector, 0, SECTOR_SIZE); // set sector to zero
        int writeError = sector_write(f, i, sector); //write sector
        if(writeError) {
			fclose(f); // close file
            return writeError; // propagate error
        }
    }
    if(!fclose(f)) { // closed
        return 0;
    } else { // error upon closing
        return ERR_IO;
    }
}
