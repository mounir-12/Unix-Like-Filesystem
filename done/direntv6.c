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
    } else {
        memset(d->dirs, 0, DIRENTRIES_PER_SECTOR * sizeof(struct direntv6)); // default value for directory's children (inode number = 0 (not used))

        d-> cur = 0; // default value
        d->last = 0; // default value

        struct inode* dirInode = &((d->fv6).i_node); // pointer to directory inode
        uint32_t size = inode_getsize(dirInode); // directory content size

        uint8_t data[inode_getsectorsize(dirInode)];

        int read = 0;
        // read directory content
        do {
            read = filev6_readblock(&(d->fv6), &(data[(d->fv6).offset]));

        } while(read > 0);

        if(read < 0) { // error occured when reading directory content
            return read; // return error
        }

        // no error
        uint32_t childNumber = 0; // child number

        for(int i = 0; i < size && childNumber < DIRENTRIES_PER_SECTOR ; i++) { // iterate as long as data remains and last child not reached
            uint32_t remaining = size - i; // remaining data to be read

            if(remaining >= sizeof(uint16_t)) { // check if a child (represrented by it inode number) is remaining
                // read child inode number
                uint16_t i_num = (data[i+1] << 8) + data[i]; // inode_number of child: read 2 bytes
                i += 2;
                (d->dirs)[childNumber].d_inumber = i_num; // write inode_number

                // read child filename
                // read as long as data remains and fileName end not reached
                for(int j = 0; i < size && j < DIRENT_MAXLEN; j++, i++) {
                    (d->dirs)[childNumber].d_name[j] = data[i]; // write character

                    if(data[i] == '\0') { // end of child name reached after reading k <= DIRENT_MAXLEN character
                        j = DIRENT_MAXLEN; // end iterations
                    } else if( j+1 == DIRENT_MAXLEN) { // DIRENT_MAXLEN characters were read
                        (d->dirs)[childNumber].d_name[j+1] = '\0'; // end fileName with nul character
                    }

                }

            }

            childNumber++; // update child number
        }
        return 0;
    }
}

int direntv6_readdir(struct directory_reader *d, char *name, uint16_t *child_inr)
{
    M_REQUIRE_NON_NULL(d);
    M_REQUIRE_NON_NULL(name);
    M_REQUIRE_NON_NULL(child_inr);

    /* Read all memoized sector */
    if(d->cur == d->last) {
        uint8_t dirs[SECTOR_SIZE];

        int read = filev6_readblock(&(d->fv6), &dirs);

        /* error occured or end of file was reached */
        if(read <= 0) {
            return read;
        }

        /* update last child read */
        d->last += read / sizeof(struct direntv6);

        /* update memoized childs sector */
        memcpy(&(d->dirs), &dirs, read);
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
            printf("%d\n", read);
            // no error
            if(read > 0) {

                size_t prefixSize = strlen(prefix);
                size_t childNameSize = strlen(childName);
                char newPrefix[prefixSize + childNameSize + 1];
                
                snprintf(newPrefix,MAXPATHLEN_UV6, "%s/%s", prefix, childName); // generate newPrefix

                // recursively call direntv6_print_tree on child
                int error = direntv6_print_tree(u, child_nr, newPrefix);

                if(error) {
                    debug_print("%s\n", ERR_MESSAGES[error - ERR_FIRST]);
                    return error;
                }
            }

        } while(read > 0);
        
        return read;
	}
}
