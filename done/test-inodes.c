#define DEBUG 1

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

int test_inode_read(const struct unix_filesystem *u, uint16_t inr)
{
	struct inode n;
	int error = inode_read(u, inr, &n);
	if(error)
	{
		debug_print("%s\n", ERR_MESSAGES[error - ERR_FIRST]);
		return error;
	}
	else
	{
		inode_print(&n);
		return 0;
	}
}

int test_inode_findsector(const struct unix_filesystem *u, uint16_t inr, int32_t file_sec_off)
{
	struct inode n;
	int error = inode_read(u, inr, &n);
	if(error)
	{
		debug_print("%s\n", ERR_MESSAGES[error - ERR_FIRST]);
		return error;
	}
	else
	{
		int sectorNb = inode_findsector(u,&n,file_sec_off);
		if(sectorNb < 0)
		{
			debug_print("%s\n", ERR_MESSAGES[sectorNb - ERR_FIRST]);
			return error;
		}
		printf("sector number: %d\n", sectorNb);
		return 0;
	}
	
}
