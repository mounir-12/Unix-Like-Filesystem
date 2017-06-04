#define DEBUG 1

#include "direntv6.h"
#include "error.h"
#include "inode.h"

int test(struct unix_filesystem *u)
{
    return direntv6_print_tree(u, ROOT_INUMBER, "");
}
