#include "sector.h"
#include "error.h"

#define SECTOR_SIZE 512

int sector_read(FILE *f, uint32_t sector, void *data)
{
	M_REQUIRE_NON_NULL(f); // return error message if f == NULL
	M_REQUIRE_NON_NULL(data); // return error message if data == NULL
	
	fseek(f, SECTOR_SIZE * sector, SEEK_SET); // move cursor SECTOR_SIZE * sector bytes from the beginning of the disk (sector start point)
	int elemRead = fread(data, sizeof(uint8_t), SECTOR_SIZE, f); // read SECTOR_SIZE bytes from f to data
	
	if(elemRead == SECTOR_SIZE) // no error
	{
		return 0;
	}
	else // not enough elements read
	{
		return ERR_IO;
	}
	
}
