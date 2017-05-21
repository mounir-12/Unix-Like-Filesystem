#include <stdio.h>
#include <string.h>
#include "inode.h"
#include "filev6.h"
#include "sector.h"
#include "error.h"
#include "bmblock.h"

int filev6_writesector(struct unix_filesystem *u, struct filev6 *fv6, const char *buf, int len, int offset);

int filev6_open(const struct unix_filesystem *u, uint16_t inr, struct filev6 *fv6)
{
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(fv6);

    struct inode n;
    int error = inode_read(u, inr, &n); // read inode

    if(error) { // error occured
        return error; // propagate error
    }

    // initialise file
    fv6->u = u;
    fv6->i_number = inr;
    fv6->i_node = n;
    fv6->offset = 0;

    return 0;


}

int filev6_readblock(struct filev6 *fv6, void *buf)
{
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(buf);

    uint32_t size = inode_getsize(&(fv6->i_node));

    if(fv6->offset >= size) {
        return 0;
    } else {
        /* sector to read from */
        int sector = inode_findsector(fv6->u, &(fv6->i_node), fv6->offset / SECTOR_SIZE);

        /* an error occured while finding the sector */
        if(sector < 0) {
            return sector; // propagate error
        } else {
            int error = sector_read((fv6->u)->f, sector, buf);

            /* an error occured while reading the sector */
            if(error) {
                return error;
            } else {

                uint32_t remainingBytes = size - fv6->offset;
                if(remainingBytes < SECTOR_SIZE) {
                    fv6->offset += remainingBytes;
                    return remainingBytes;
                } else {
                    fv6->offset += SECTOR_SIZE;
                    return SECTOR_SIZE;
                }
            }
        }
    }
}

int filev6_lseek(struct filev6 *fv6, int32_t offset)
{
    M_REQUIRE_NON_NULL(fv6);

    uint32_t size = inode_getsize(&(fv6->i_node)); // size of file

    if(offset >= size) { // offset out of range
        return ERR_OFFSET_OUT_OF_RANGE; // return error
    }

    fv6->offset = offset;
    return 0;
}

int filev6_create(struct unix_filesystem *u, uint16_t mode, struct filev6 *fv6)
{
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(fv6);

    int bit = bm_get(u->ibm, fv6->i_number); // get corresponding bit
    if(bit < 0) { // inode is not in the [min;max] range
        return bit; // propagate error
    }
    if(bit == 0) { // inode is unallocated
        return ERR_UNALLOCATED_INODE; // return appropriate error code
    }

    struct inode i; // inode to be written
    memset(&i, 0, sizeof(struct inode)); // set all values to zero
    i.i_mode = mode; // correctly set the i_mode

    int error = inode_write(u, fv6->i_number, &i); // write the inode
    if(error) { // error occured
        return error; // propagate error
    }
    fv6->i_node = i; // set the inode

    return 0;
}

int filev6_writebytes(struct unix_filesystem *u, struct filev6 *fv6, const void *buf, int len)
{
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(buf);
    if(len < 0) { // number of bytes to be written < 0
        return ERR_BAD_PARAMETER; // return error
    }

    int written = 0; // number of bytes written
    uint32_t size = inode_getsize(&(fv6->i_node)); // file size

    while(written < len) { // keep writing sectors untill writing the full buffer
        int nbWritten = filev6_writesector(u, fv6, buf, len, written);
        if(nbWritten < 0) { // error occured
            return nbWritten; // propagate error
        }
        written += nbWritten; // update total written bytes
        size += nbWritten; // update size
        int error = inode_setsize(&(fv6->i_node), size); // update inode size
        if(error) // an error occured
        {
			return error; // propagate error
		}
    }
    // finished writing file content
    int error = inode_write(u, fv6->i_number, &(fv6->i_node)); // write inode to update size and addresses array
    if(error) { // error occured
        return error; // propagate error
    }
    return 0;
}

int filev6_writesector(struct unix_filesystem *u, struct filev6 *fv6, const char *buf, int len, int offset)
{
    uint32_t size = inode_getsize(&(fv6->i_node)); // file size
    uint32_t smallFileMaxSize = ADDR_SMALL_LENGTH * SECTOR_SIZE; // small file is 8 * 512 bytes = 4 Kbytes
    uint32_t largeFileMaxSize = (ADDR_SMALL_LENGTH - 1) * ADDRESSES_PER_SECTOR * SECTOR_SIZE; // large file is 7 * 256 * 512 bytes = 896 Kbytes
    uint32_t remaining = SECTOR_SIZE - (size % SECTOR_SIZE); // remaining bytes in last occupied sector
    
    // nb_bytes is the minimum between the remaining bytes and the number
    //of bytes to be written = len - offset
    uint32_t nb_bytes = (remaining < (len - offset)) ? remaining : (len - offset); // number of bytes to be written
    char block[SECTOR_SIZE]; // the block to be written
    memset(block, 0, SECTOR_SIZE); // initialize block
    int error = 0; // possible error to propagate

    if(size < smallFileMaxSize) {
        int sector = 0; //sector number
        if(size % SECTOR_SIZE == 0) { // file size is a multiple of SECTOR_SIZE
            int sector = bm_find_next(u->fbm); // find next free sector number
            if(sector < 0) { // no free sector
                return sector; // propagate error
            }
            bm_set(u->fbm, sector); // set the data sector to be allocated
            memcpy(block, &(buf[offset]), nb_bytes); // copy bytes to be written (starting from offset)
            uint16_t addressIndex = size / SECTOR_SIZE; // index of the new sector number in inode's addresses
            (fv6->i_node).i_addr[addressIndex] = sector; // add the new sector number to the array
        } else { // file size is not a multiple of SECTOR_SIZE
            uint16_t addressIndex = size / SECTOR_SIZE; // index of the last sector number in inode's addresses
            sector = (fv6->i_node).i_addr[addressIndex]; // last sector in the file
            error = sector_read(u->f, sector, block); // read sector
            if(error) { // error occured
                return error; // propagate error
            }
            uint32_t nextByteIndex = size % SECTOR_SIZE; // index inside the block of the next byte to be written to block
            memcpy(&(block[nextByteIndex]), &(buf[offset]), nb_bytes); // write subsequently nb_bytes to the block
        }
        error = sector_write(u->f, sector, block); // write block
        if(error) { // an error occured
            return error; // propagate error
        }
        return nb_bytes; // return number of bytes written
    } else if(size < largeFileMaxSize) { // file is a large File
        char sector[SECTOR_SIZE]; // undirect sector
        memset(sector, 0, SECTOR_SIZE); // initialize sector
        int undirectSector = 0; //undirect sector number
        int directSector = 0; //direct sector number

        if(size == smallFileMaxSize) {

            memcpy(sector, (fv6->i_node).i_addr, ADDR_SMALL_LENGTH * ADDRESS_SIZE); // copy direct addresses to the undirect sector

            undirectSector = bm_find_next(u->fbm); // find next free sector number
            if(undirectSector < 0) { // no free sector
                return undirectSector; // propagate error
            }
            memset((fv6->i_node).i_addr, 0, ADDR_SMALL_LENGTH); // set array values to 0
            (fv6->i_node).i_addr[0] = undirectSector; // add the first undirect sector number to the array
        }

        if(size % SECTOR_SIZE == 0) {
            uint16_t undirectAddressIndex = size / (ADDRESSES_PER_SECTOR + SECTOR_SIZE); // index of the current undirect sector number in inode's addresses
            uint16_t directAddressIndex = size % SECTOR_SIZE; // index of the direct sector number in undirect sector addresses

            if(size % (ADDRESSES_PER_SECTOR + SECTOR_SIZE)) {
                undirectSector = bm_find_next(u->fbm); // find next free undirect sector number
                if(undirectSector < 0) { // no free sector
                    return undirectSector; // propagate error
                }

                directSector = bm_find_next(u->fbm); // find next free direct sector number
                if(directSector < 0) { // no free sector
                    return directSector; // propagate error
                }
                sector[directAddressIndex] = directSector;

                (fv6->i_node).i_addr[undirectAddressIndex] = undirectSector; // add the next undirect sector number to the array
            } else {
                undirectSector = (fv6->i_node).i_addr[undirectAddressIndex]; // last undirect sector
                error = sector_read(u->f, undirectSector, sector); // read undirect sector
                if(error) { // error occured
                    return error; // propagate error
                }
                directSector = bm_find_next(u->fbm); // find next free direct sector number
                if(directSector < 0) { // no free sector
                    return directSector; // propagate error
                }
            }

            sector[directAddressIndex] = directSector;
            error = sector_write(u->f, undirectSector, sector); // write undirect sector
            if(error) { // an error occured
                return error; // propagate error
            }

            memcpy(block, &(buf[offset]), nb_bytes); // copy bytes to be written (starting from offset)

        } else {
            undirectSector = (fv6->i_node).i_addr[ADDR_SMALL_LENGTH - 2]; // last undirect sector
            error = sector_read(u->f, undirectSector, sector); // read undirect sector
            if(error) { // error occured
                return error; // propagate error
            }
            directSector = sector[ADDRESSES_PER_SECTOR - 1]; // last direct sector
            error = sector_read(u->f, directSector, block); // read direct sector
            if(error) { // error occured
                return error; // propagate error
            }
            uint32_t nextByteIndex = size % SECTOR_SIZE; // index inside the block of the next byte to be written to block
            memcpy(&(block[nextByteIndex]), &(buf[offset]), nb_bytes); // write subsequently nb_bytes to the block
        }

        error = sector_write(u->f, directSector, block); // write direct sector
        if(error) { // an error occured
            return error; // propagate error
        }

        return nb_bytes; // return number of bytes written
    } else { // file is too large
        return ERR_FILE_TOO_LARGE;
    }

}
