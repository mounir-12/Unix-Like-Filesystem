#include "mount.h"
#include "inode.h"
#include "error.h"
#include "unixv6fs.h"

int test(struct unix_filesystem *u)
{
	return inode_scan_print(u);
}

int test_inode_print()
{
	struct inode n;
	n.i_mode = 32768;
	n.i_nlink = 0;
	n.i_uid = 0;
	n.i_gid = 0;
	n.i_size0 = 0;
	n.i_size1 = 18;
	inode_print(&n);
	printf("\n");
	inode_print(NULL);
	return 0;
}
