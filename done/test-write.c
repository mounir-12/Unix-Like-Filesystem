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
	fv6.i_number = 3;
	fv6.offset = 0;
	filev6_create(u, DIR, &fv6);
	return inode_scan_print(u);
}
