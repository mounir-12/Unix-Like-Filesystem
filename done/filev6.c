#include <stdio.h>
#include "filev6.h"
#include "error.h"

int filev6_open(const struct unix_filesystem *u, uint16_t inr, struct filev6 *fv6)
{
	M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(fv6);

    struct inode;
    int error = inode_read(u, inr, &inode);

    if(error) {
        return error;
    }

    fv6->u = u;
    fv6->i_number = inr;
    fv6->inode = inode;
    fv6->offset = 0;
    
    return 0;
	
	
}

int filev6_readblock(struct filev6 *fv6, void *buf)
{
	M_REQUIRE_NON_NULL(fv6);
	M_REQUIRE_NON_NULL(buf);
	
	if(fv6->offset >= inode_getsize(fv6->i_node)){
		fv6->offset = 0;
		return 0;
	} else {
		
		/* sector to read from */
		int sector = inode_findsector(fv6->u, fv6->i_node, fv6->offset);
		
		/* an error occured while finding the sector */
		if(sector < 0){
			return sector;
		} else {
			int error = sector_read((fv6->u).f, sector, buf);
			
			/* an error occured while reading the sector */
			if(error){
				return error;
			} else {
				fv6->offset += SECTOR_SIZE;
				
				if(){
				}
			}
		}
	}
	
}
