#define DEBUG 1

#include "direntv6.h"
#include "error.h"
#include "inode.h"

void direntv6_opendir_test(const struct unix_filesystem *u, uint16_t inr, struct directory_reader *d);

int test(struct unix_filesystem *u)
{
	struct directory_reader d;
	uint16_t inr = 1;
	direntv6_opendir_test(u, inr, &d);
}

void direntv6_opendir_test(const struct unix_filesystem *u, uint16_t inr, struct directory_reader *d)
{
	int error = direntv6_opendir(u,inr,d); // read directory
	
	if(error) // error occured
	{
		debug_print("%s\n", ERR_MESSAGES[error - ERR_FIRST]);
		return;
	}
	
	// directory read
	for(int i = 0; i < DIRENTRIES_PER_SECTOR; i++)
	{
		uint16_t i_num = (d->dirs[i]).d_inumber; // inode number
		
		struct inode n;
		error = inode_read(u, i_num, &n); // read inode
		
		if(error) // error occured
		{
			return;
		}
		
		// no error
		if(n.i_mode & IALLOC) // allocated inode
		{
				/* check wether inode is a directory or a file */
                if (n.i_mode & IFDIR) {
                    printf("%s ", SHORT_DIR_NAME);
                } else {
                    printf("%s ", SHORT_FIL_NAME);
                }
                printf("%s\n", (d->dirs[i]).d_name);
		}
	}
}
