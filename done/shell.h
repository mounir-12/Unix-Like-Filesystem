/**
 * @file shell.h
 * @brief executing commands specific to unix filesystem
 *
 * @author Mounir Amrani & Maximilien Ulysse Arthur Groh
 * @date April 2017
 */

#include "mount.h"
#include "error.h"
#include "inode.h"
#include "direntv6.h"
#include "unixv6fs.h"

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
 * @param args absolute path of the disk to be mounted
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
 * @param args absolute path of the file
 * @return 0 on success; >0 on error
 */
int do_cat(char** args);

/**
 * @brief prints inode number and sha of the content of the file
 * @param args absolute path of the file
 * @return 0 on success; >0 on error
 */
int do_sha(char** args);

/**
 * @brief prints inode number of the file or directory
 * @param args absolute path of the file or directory
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
 * @param args disk name, number of sectors and numbers of inodes
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
 * @param args name of the local file and name of the destination file in the mounted unix filesystem 
 * @return 0 on success; >0 on error
 */
int do_add(char** args);

/**
 * @brief tokenizes the input using spaces
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
