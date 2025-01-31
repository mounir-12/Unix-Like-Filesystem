#include <stdio.h>
#include "inode.h"
#include "filev6.h"
#include "sector.h"
#include "error.h"

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
    // correcteur: copie inutile, autant écrire directement dans la structure
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
    
    if(fv6->offset >= size) 
    {
        return 0;
    } 
    else 
    {
        /* sector to read from */
        int sector = inode_findsector(fv6->u, &(fv6->i_node), fv6->offset / SECTOR_SIZE);

        /* an error occured while finding the sector */
        if(sector < 0) 
        {
            return sector;
        }
        else
        {
            int error = sector_read((fv6->u)->f, sector, buf);
            
            /* an error occured while reading the sector */
            if(error)
            {
                return error;
            }
            else
            {

                uint32_t remainingBytes = size - fv6->offset;
                if(remainingBytes < SECTOR_SIZE) 
                {
                    fv6->offset += remainingBytes;
                    return remainingBytes;
                }
                else
                {
                    fv6->offset += SECTOR_SIZE;
                    return SECTOR_SIZE;
                }
            }
        }
    }

}
