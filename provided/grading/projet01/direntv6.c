#include "direntv6.h"
#include "error.h"
#include "filev6.h"
#include "inode.h"
#include <string.h>

# define MAXPATHLEN_UV6 1024
int direntv6_dirlookup_core(const struct unix_filesystem *u, uint16_t inr, char *entry, size_t length);

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

        int read = filev6_readblock(&(d->fv6), &(d->dirs)); // read at most a block - next block

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
        // correcteur: on se serait aisément passé du dernier paramètre.
        printf("%s %s%s\n", SHORT_DIR_NAME, prefix, "/");

        int read = 0;

        // Iterate on all childs of the current directory // correcteur: children
        do {
            char childName[DIRENT_MAXLEN+1];
            uint16_t child_inr;

            read = direntv6_readdir(&d, childName, &child_inr);

            // no error
            if(read > 0) {

                size_t prefixSize = strlen(prefix);
                size_t childNameSize = strlen(childName);
                char newPrefix[prefixSize + childNameSize + 1]; // correcteur: il manque la place pour le '\0'


                snprintf(newPrefix,MAXPATHLEN_UV6, "%s%s%s", prefix, "/", childName); // generate newPrefix

                // recursively call direntv6_print_tree on child
                int error = direntv6_print_tree(u, child_inr, newPrefix);

                if(error) {
                    return error;
                }
            }

        } while(read > 0);

        return read;
    }
}

int direntv6_dirlookup(const struct unix_filesystem *u, uint16_t inr, const char *entry)
{
    // check for non NULL arguments
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(entry);
    
    // correcteur: il manque la place pour le '\0'
    char entryCopy[strlen(entry)]; // copy which is non const
    // correcteur: strncpy
    sprintf(entryCopy, "%s", entry); // copy content

    return direntv6_dirlookup_core(u, inr, entryCopy, strlen(entryCopy));
}

int direntv6_dirlookup_core(const struct unix_filesystem *u, uint16_t inr, char *entry, size_t length)
{
    if(length == 0) { // found the file or directory
        return inr; // return its inode
    }

    if(entry[0] == '/') { // found a '/' in the beginning
        // correcteur: Il y a plus efficace comme façon de faire mais OK
        return direntv6_dirlookup_core(u, inr, &(entry[1]), length-1); // ignore it
    }

    char* lastCharName = strchr(entry, '/'); // search for '/' and save a pointer to it
    char* nextEntry = &(entry[length]); // pointer to the next char after the '/', initialized to point to the terminating \0

    if(lastCharName != NULL) { // found the '/'
        *lastCharName = '\0'; // end of name
        nextEntry = lastCharName+1; // point to next char
    }

    struct directory_reader d;
    int openingError = direntv6_opendir(u, inr, &d); // open current directory using its inode number inr

    if(openingError) { // if not on a directory or other error occured
        return openingError; // propagate the error
    }
    
    int read = 0;
    // Iterate on all childs of the current directory
    do {
        char childName[DIRENT_MAXLEN+1]; // child name
        uint16_t child_inr; // child inode

        read = direntv6_readdir(&d, childName, &child_inr); // read child

        if(read < 0) { // error occured while reading child
            return read; // propagate error
        } else if(read > 0) { // succefully read child
            // correcteur: attention aux lectures au delà de la mémoire. 
            // Exemple de scénario d'exploitation de cette faille:
            // Je crée un dossier contenant un fichier avec un très long nom. Puis je fais
            // une requête avec un très petit nom (ex: `ls a`). Alors 'strcmp' lance une comparaison
            // entre le nom de mon dossier et les données contenues dans la stack du processus de votre
            // programme. Si les conditions sont reproductibles, je peux changer le nom du fichier et
            // "deviner" votre stack caractère par caractère.

            if(strcmp(entry, childName) == 0) { // found the searched subdir or fil
                return direntv6_dirlookup_core(u, child_inr, nextEntry, strlen(nextEntry)); //
            }
        }

    } while(read > 0);

    // no more children => fil or subdir not found
    return ERR_INODE_OUTOF_RANGE;

}
