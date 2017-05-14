#define DEBUG 1

#include "mount.h"
#include "inode.h"
#include "error.h"
#include "filev6.h"
#include "unixv6fs.h"

int test(struct unix_filesystem *u)
{
	uint16_t DIR = IALLOC | IFDIR; // allocated directory
	uint16_t FIL = IALLOC | IFMT; // allocated file
	struct filev6 fv6;
	fv6.u = u;
	fv6.i_number = 4;
	fv6.offset = 0;
	int error = filev6_create(u, DIR, &fv6);
	if(error)
	{
		debug_print("%s\n", ERR_MESSAGES[error - ERR_FIRST]);
	}
	return inode_scan_print(u);
}
