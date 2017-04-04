#define DEBUG 1

#include "direntv6.h"
#include "error.h"
#include "filev6.h"
#include "inode.h"
#include <string.h>

# define MAXPATHLEN_UV6 1024

int direntv6_opendir(const struct unix_filesystem *u, uint16_t inr, struct directory_reader *d)
{
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(d);

    struct inode n;
    int error = inode_read(u, inr, &n); // read inode

    if(error) { // error occured while reading inode
        return error;
    }

    if((n.i_mode & IFMT) != IFDIR) { // inode is not a directory
        return ERR_INVALID_DIRECTORY_INODE; // return appropriate error code
    }

    // inode is a directoy
    error = filev6_open(u, inr, &(d->fv6)); // open directory

    if(error) { // error occured
        return error; // propagate the error
    }
    
    d->last = 0;
    d->cur = 0;
    return 0;
}

int direntv6_readdir(struct directory_reader *d, char *name, uint16_t *child_inr)
{
    M_REQUIRE_NON_NULL(d);
    M_REQUIRE_NON_NULL(name);
    M_REQUIRE_NON_NULL(child_inr);

    /* Read all memoized sector */
    if(d->cur == d->last) {

        int read = filev6_readblock(&(d->fv6), &(d->dirs)); // read at most a block

        /* error occured or end of file was reached */
        if(read <= 0) {
            return read;
        }

        /* update last child read */
        d->last = read / sizeof(struct direntv6);

        d->cur = 0;

    }
    struct direntv6 child = d->dirs[d->cur];

    /* output child number read */
    *child_inr = child.d_inumber;

    /* output child name read */
    strncpy(name, child.d_name, DIRENT_MAXLEN);
    name[DIRENT_MAXLEN] = '\0';

    /* update current child */
    d->cur++;

    return 1;
}

int direntv6_print_tree(const struct unix_filesystem *u, uint16_t inr, const char *prefix)
{
    struct directory_reader d;
    int openingError = direntv6_opendir(u, inr, &d);

    /* current node isn't a directory */
    if(openingError == ERR_INVALID_DIRECTORY_INODE) {
        /* print file path */
        printf("%s %s\n", SHORT_FIL_NAME, prefix);
        return 0;
    }

    /* error occured while opening the directory */
    else if(openingError) {
        return openingError;
    }

    /* current node is a directory */
    else {

        /* print directory path */
        printf("%s %s\n", SHORT_DIR_NAME, prefix);

        int read = 0;

        // Iterate on all childs of the current directory
        do {
            char childName[DIRENT_MAXLEN];
            uint16_t child_nr;

            read = direntv6_readdir(&d, childName, &child_nr);

            // no error
            if(read > 0) {

                size_t prefixSize = strlen(prefix);
                size_t childNameSize = strlen(childName);
                char newPrefix[prefixSize + childNameSize + 1];


                snprintf(newPrefix,MAXPATHLEN_UV6, "%s%s/", prefix, childName); // generate newPrefix

                // recursively call direntv6_print_tree on child
                int error = direntv6_print_tree(u, child_nr, newPrefix);

                if(error) {
                    return error;
                }
            }

        } while(read > 0);

        return read;
    }
}
