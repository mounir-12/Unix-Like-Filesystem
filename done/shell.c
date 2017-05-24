/**
 * @file shell.c
 * @brief executing commands specific to unix filesystem
 *
 * @author Mounir Amrani & Maximilien Ulysse Arthur Groh
 * @date April 2017
 */

#include "mount.h"
#include "error.h"
#include "inode.h"
#include "direntv6.h"
#include "sha.h"
#include "unixv6fs.h"
#include <string.h>

#define CMD_NUM 13
#define MAX_CHARS 255
#define MAX_ARGS 3

enum shell_codes {
    SHELL_FIRST = 1, // not an actual error but to set the first error number
    SHELL_INVALID_COMMAND,
    SHELL_INVALID_ARGS,
    SHELL_UNMOUNTED_FS,
    SHELL_CAT_ON_DIR,
    SHELL_LAST // not an actual error but to have e.g. the total number of errors
};

// pointer on the function to execute
typedef int (*shell_fct)(char**);

struct shell_map {
    const char* name; // command name
    shell_fct fct; // function to perform the command
    const char* help; // command description
    size_t argc; // command arguments number
    const char* args; // command arguments description
};

/**
 * @brief exits the shell
 * @param args not used
 * @return 0 on success; >0 on error
 */
int do_exit(char** args);

/**
 * @brief quits the shell
 * @param args not used
 * @return 0 on success; >0 on error
 */
int do_quit(char** args);

/**
 * @brief prints all supported commands with their description
 * @param args not used
 * @return 0 on success; >0 on error
 */
int do_help(char** args);

/**
 * @brief mounts the disk
 * @param args path of the disk to be mounted
 * @return 0 on success; >0 on error
 */
int do_mount(char** args);

/**
 * @brief lists all files and directories in the mounted filesystem
 * @param args not used
 * @return 0 on success; >0 on error
 */
int do_lsall(char** args);

/**
 * @brief prints the superblock of the mounted filesystem
 * @param args not used
 * @return 0 on success; >0 on error
 */
int do_psb(char** args);

/**
 * @brief prints content of the file
 * @param args absolute path of the file in the mounted disk
 * @return 0 on success; >0 on error
 */
int do_cat(char** args);

/**
 * @brief prints inode number and sha of the content of the file
 * @param args absolute path of the file in the mounted disk
 * @return 0 on success; >0 on error
 */
int do_sha(char** args);

/**
 * @brief prints inode number of the file or directory
 * @param args absolute path of the file or directory in the mounted disk
 * @return 0 on success; >0 on error
 */
int do_inode(char** args);

/**
 * @brief reads and prints the inode of the inode number
 * @param args inode number
 * @return 0 on success; >0 on error
 */
int do_istat(char** args);

/**
 * @brief create a new unix filesystem
 * @param args disk name - number of sectors - numbers of inodes
 * @return 0 on success; >0 on error
 */
int do_mkfs(char** args);

/**
 * @brief creates a new directory in the mounted unix filesystem
 * @param args directory name
 * @return 0 on success; >0 on error
 */
int do_mkdir(char** args);

/**
 * @brief adds a local file to the mounted unix filesystem
 * @param args name of the local file - name of the destination file in the mounted disk
 * @return 0 on success; >0 on error
 */
int do_add(char** args);

/**
 * @brief tokenizes the input using the character ' ' (space)
 * @param input the input to tokenise (IN)
 * @param tokenized the tokenized input (OUT)
 * @return 0 on success; >0 on error
 */
int tokenize_input(char* input, char** tokenized);

/**
 * @brief executes command
 * @param tokenized command followed by its arguments
 * @return 0 on success; >0 or <0 on error
 */
int execute_command(char** tokenized);

/**
 * @brief displays error code in case of error
 * @param error the error code
 */
void handle_error(int error);

// shell error messages
const char * const SHELL_MESSAGES[] = {
    "", // no error
    "invalid command",
    "wrong number of arguments",
    "mount the FS before the operation",
    "cat on a directory is not defined"
};

// global array of supported shell commands
struct shell_map shell_cmds[] = {
    {"help", do_help, "display this help", 0, ""},
    {"exit", do_exit, "exit shell", 0, ""},
    {"quit", do_quit, "exit shell", 0, ""},
    {"mkfs", do_mkfs, "create a new filesystem", 3, " <diskname> <#inodes> <#blocks>"},
    {"mount", do_mount, "mount the provided filesystem", 1, " <diskname>"},
    {"mkdir", do_mkdir, "create a new directory", 1, " <dirname>"},
    {"lsall", do_lsall, "list all directories and files contained in the currently mounted filesystem", 0, ""},
    {"add", do_add, "add a new file", 2, " <src-fullpath> <dst>"},
    {"cat", do_cat, "display the content of a file", 1, " <pathname>"},
    {"istat", do_istat, "display information about the provided inode", 1, " <inode_nr>"},
    {"inode", do_inode, "display the inode number of a file", 1, " <pathname>"},
    {"sha", do_sha, "display the SHA of a file", 1, " <pathname>"},
    {"psb", do_psb, "Print SuperBlock of the currently mounted filesystem", 0, ""},
};

// global variable representing the mounted unixv6 filesystem
struct unix_filesystem u;

// global variable to signal exit request
int end = 0;

int main(void)
{
    u.f = NULL; // file is NULL (not mounted yet)
    printf("Shell interpretor\n");
    printf("Type \"help\" for more information.\n");

    while(!feof(stdin) && !ferror(stdin)) {
        char input[MAX_CHARS]; // user input
        char* tokenized[MAX_ARGS+2]; // tokenized user input +2 for the first char sequence which is the command and the last arg which is an invalid arg

        printf(">>> ");

        fgets(input,MAX_CHARS, stdin); // read user input
        if(feof(stdin)) { // pressed CTRL+D
            printf("\n");
            do_exit(NULL);
        } else {
            // otherwise
            int lastCharIndex = strlen(input) - 1; // last read char index
            if(lastCharIndex >= 0 && input[lastCharIndex] == '\n') { // if \n read
                input[lastCharIndex] = '\0'; // remove \n
            }
            tokenize_input(input, tokenized); // tokenize user input


            handle_error(execute_command(tokenized)); // execute command and handle error in case an error occured
        }

        if(end) { // check if exit command issued
            return 0;
        }
    }
    return 0;
}

int do_exit(char** args)
{
    if(u.f !=NULL) { // already mounted
        int error = umountv6(&u); // unmount
        if(error) { // error unmounting
            return error; // propagate error
        }
    }

    end = 1; // signal end
    return 0;
}

int do_quit(char** args)
{
    return do_exit(args);
}

int do_help(char** args)
{
    for(int i = 0; i < CMD_NUM; i++) {
        printf("	- %s%s: %s.\n", shell_cmds[i].name, shell_cmds[i].args, shell_cmds[i].help);

    }
    return 0;
}

int do_mount(char** args)
{
    M_REQUIRE_NON_NULL(args);

    int error = mountv6(args[0],&u); // mount the filesystem
    if(error) { // error occured while mounting
        u.f = NULL; // file is NULL (not mounted yet)
        return error; // propagate error
    }
    // mounted
    return 0;
}

int do_lsall(char** args)
{
    if(u.f == NULL) { // if filesystem not mounted
        return SHELL_UNMOUNTED_FS; // return appropriate error code
    }
    // mounted
    int error = direntv6_print_tree(&u, ROOT_INUMBER, "");
    if(error) { // error occured
        return error; // propagate error
    }
    return 0;
}

int do_psb(char** args)
{
    if(u.f == NULL) { // if filesystem not mounted
        return SHELL_UNMOUNTED_FS; // return appropriate error code
    }
    // mounted
    mountv6_print_superblock(&u);
    return 0;
}

int do_cat(char** args)
{
    M_REQUIRE_NON_NULL(args);

    if(u.f == NULL) { // if filesystem not mounted
        return SHELL_UNMOUNTED_FS; // return appropriate error code
    }
    // mounted
    int inr = direntv6_dirlookup(&u, ROOT_INUMBER, args[0]); // search inode number
    if(inr < 0) { // inode not found
        return inr; // propagate error code
    }

    struct filev6 file;
    int error = filev6_open(&u, inr, &file); // open the file
    if(error) { // error occured while opening file
        return error; // propagate error
    }

    if((file.i_node.i_mode & IFMT) == IFDIR) { // file is a directory
        return SHELL_CAT_ON_DIR; // return appropriate error code
    }
    int32_t sectorsSize = inode_getsectorsize(&(file.i_node)); // data sectors size + 1 for null character
    unsigned char data[sectorsSize]; // data of the file
    data[sectorsSize - 1] = '\0'; // null terminate the data

    int read = 0;

    // read the whole file
    do {
        read = filev6_readblock(&file, &(data[file.offset]));

    } while(read > 0);

    // print read content
    printf("%s\n", data);

    return 0;
}

int do_sha(char** args)
{
    M_REQUIRE_NON_NULL(args);

    if(u.f == NULL) { // if filesystem not mounted
        return SHELL_UNMOUNTED_FS; // return appropriate error code
    }
    // mounted
    int inr = direntv6_dirlookup(&u, ROOT_INUMBER, args[0]); // search inode number
    if(inr < 0) { // inode not found
        return inr; // propagate error code
    }

    struct inode n;
    int error = inode_read(&u, inr, &n); // read inode

    if(error) { // error occured
        return error; // propagate error
    }

    print_sha_inode(&u,n,inr);
    return 0;
}

int do_inode(char** args)
{
    M_REQUIRE_NON_NULL(args);

    if(u.f == NULL) { // if filesystem not mounted
        return SHELL_UNMOUNTED_FS; // return appropriate error code
    }
    // mounted
    int inr = direntv6_dirlookup(&u, ROOT_INUMBER, args[0]); // search inode number
    if(inr < 0) { // inode not found
        return inr; // propagate error code
    }

    printf("inode: %d\n", inr);
    return 0;
}

int do_istat(char** args)
{
    M_REQUIRE_NON_NULL(args);

    if(u.f == NULL) { // if filesystem not mounted
        return SHELL_UNMOUNTED_FS; // return appropriate error code
    }
    // mounted

    int inr = 0; // inode number to be extracted
    if(sscanf(args[0], "%d", &inr) != 1) { // args[0] has no spaces, if can't extract inode number
        return ERR_INODE_OUTOF_RANGE; // return appropriate error code
    }

    struct inode n;
    int error = inode_read(&u, inr, &n); // read inode

    if(error) { // error occured
        return error; // propagate error
    }

    inode_print(&n); // print inode

    return 0;
}

int do_mkfs(char** args)
{
    uint16_t num_blocks = 0; // number of blocks
    uint16_t num_inodes = 0; // number of inodes
    int read = sscanf(args[1], "%hu", &num_inodes); // extract the number of inodes
    if(read != 1) { // error extracting
        return SHELL_INVALID_ARGS; // return appropriate error code
    }

    read = sscanf(args[2], "%hu", &num_blocks); // extract number of blocks
    if(read != 1) { // error extracting
        return SHELL_INVALID_ARGS; // return appropriate error code
    }

    int error = mountv6_mkfs(args[0], num_blocks, num_inodes); // make a new file system
    if(error) { // error occured
        return error; // propagate error
    }
    return 0; // no error
}

int do_mkdir(char** args)
{
    M_REQUIRE_NON_NULL(args);

    if(u.f == NULL) { // if filesystem not mounted
        return SHELL_UNMOUNTED_FS; // return appropriate error code
    }
    // mounted


    uint16_t DIR = IALLOC | IFDIR; // allocated directory
    int error = direntv6_create(&u,args[0], DIR); // create a directory
    if(error) { // error occured
        return error; // propagate error
    }
    return 0;
}

int do_add(char** args)
{
    M_REQUIRE_NON_NULL(args);

    if(u.f == NULL) { // if filesystem not mounted
        return SHELL_UNMOUNTED_FS; // return appropriate error code
    }
    // mounted
    FILE* file = fopen(args[0], "rb");
    if(file == NULL) { // error occured
        return ERR_IO; // return appropriate error code
    }
    uint16_t FIL = IALLOC; // allocated file
    int error = direntv6_create(&u, args[1], FIL); // create a new file in filesystem
    if(error) { // error occured while creating file
        return error; // propagate error
    }
    fseek(file, 0, SEEK_END); // go to end of file
    int size = ftell(file); // get file size
    if(size < 0) { // error occured
        return ERR_IO; // propagate error
    }
    uint8_t data[size]; // buffer to contain the file data
    fseek(file, 0, SEEK_SET); // seek the beginning of file
    fread(data, sizeof(uint8_t), size, file); // read the whole file
    int fileInr = direntv6_dirlookup(&u, ROOT_INUMBER, args[1]); // search inode number of new file
    struct filev6 newFile; // filev6 for the new file
    error = filev6_open(&u, fileInr, &newFile); // open filev6
    if(error) { // error occured
        return error; // propagate error
    }
    error = filev6_writebytes(&u, &newFile, data, size); // write data to file
    if(error) { // error occured
        return error; // propagate error
    }
    fclose(file);
    return 0;
}

int tokenize_input(char* input, char** tokenized)
{
    M_REQUIRE_NON_NULL(input); // return error code if NULL
    M_REQUIRE_NON_NULL(tokenized); // return error code if NULL

    tokenized[0] = strtok(input, " "); // compute the first token
    for(int i = 1; i < MAX_ARGS+2; i++) { // compute next tokens
        tokenized[i] = strtok(NULL, " ");
    }
    return 0;
}

int execute_command(char** tokenized)
{
    M_REQUIRE_NON_NULL(tokenized); // return error if argument is NULL

    if(tokenized[0] == NULL) { // no command
        return SHELL_INVALID_COMMAND; // return appropriate error code
    }

    int index = -1; // index of the entered command in the shell_cmds array
    for(int i=0; i < CMD_NUM; i++) {
        if(strcmp(tokenized[0], shell_cmds[i].name) == 0) { // found command
            index = i; // set index
        }
    }
    if(index < 0) { // invalid index, so not a command
        return SHELL_INVALID_COMMAND; // return appropriate error code
    }
    int argsNb = 0;// number of arguments
    for(int i = 0; i < MAX_ARGS+1; i++) { // count the number of arguments
        if(tokenized[i+1] != NULL) { // found a valid argument
            argsNb++; // increase number of arguments
        }
    }
    if(argsNb != shell_cmds[index].argc) { // not the expected number of arguments
        return SHELL_INVALID_ARGS; // return appropriate error code
    }
    return shell_cmds[index].fct(&tokenized[1]); // execute command and return its error code

}

void handle_error(int error)
{
    if(error > 0) { // SHELL ERROR
        printf("ERROR SHELL: %s\n", SHELL_MESSAGES[error - SHELL_FIRST]);
    } else if(error < 0) { // FS error
        printf("ERROR FS: %s", ERR_MESSAGES[error - ERR_FIRST]);
        if(error == ERR_IO) { // IO ERROR
            printf(": No such file or directory");
        }
        printf("\n");
    }
}

