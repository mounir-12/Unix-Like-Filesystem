#define DEBUG 1

#include "mount.h"
#include "inode.h"
#include "error.h"
#include "bmblock.h"
#include "unixv6fs.h"

int test(struct unix_filesystem *u)
{
	bm_print(u->fbm);
	bm_print(u->ibm);
	return 0;
}

