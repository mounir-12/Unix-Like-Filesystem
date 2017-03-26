#include "mount.h"
#include "sector.h"
#include "error.h"
#include <string.h>
#include <inttypes.h>

int mountv6(const char *filename, struct unix_filesystem *u)
{
	M_REQUIRE_NON_NULL(filename);
	M_REQUIRE_NON_NULL(u);
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

void mountv6_print_superblock(const struct unix_filesystem *u)
{
	if(u == NULL)
	{
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
	if(!fclose(u->f)) // closed
	{
		return 0;
	}
	else // error upon closing
	{
		return ERR_IO;
	}
	
}
