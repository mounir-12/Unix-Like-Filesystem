#include "mount.h"
#include "sector.h"
#include "error.h"
#include <string.h>

int mountv6(const char *filename, struct unix_filesystem *u)
{
	//init u
	memset(u, 0, sizeof(*u));
	u->fbm = NULL;
	u->ibm = NULL;
	
	u->f = fopen(filename,"rw"); // open file in u->f in binary read mode
	if(u->f == NULL) // open error
	{
		return ERR_IO;
	}
	else
	{
		uint8_t bootBlock[SECTOR_SIZE];
		int error = sector_read(u->f,BOOTBLOCK_SECTOR,bootBlock); // read boot block sector
		
		if(!error && bootBlock[BOOTBLOCK_MAGIC_NUM_OFFSET] != BOOTBLOCK_MAGIC_NUM) // no read error but magic num not found
		{
			return ERR_BADBOOTSECTOR;
		}
		else if(error) // read error
		{
			return error;
		}
		else // correctly mounted
		{
			return sector_read(u->f,SUPERBLOCK_SECTOR,&(u->s)); // read and return the returned value: 0 if success, error otherwise
		}
	}
}
