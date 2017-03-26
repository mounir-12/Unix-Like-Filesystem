#include "mount.h"
#include "inode.h"
#include "error.h"

int test(struct unix_filesystem *u)
{
	M_REQUIRE_NON_NULL(u);
	inode_scan_print(u);
	return 0;
}
