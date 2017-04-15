#include "shell.h"
#include <string.h>

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
    while(!feof(stdin) && !ferror(stdin)) {
        char input[MAX_CHARS]; // user input
        char* tokenized[MAX_ARGS+1]; // tokenized user input

        printf(">");

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


            handle_error(execute_command(tokenized), tokenized[0]); // execute command and handle error in case an error occured
        }

        if(end) { // check if exit command issued
            return 0;
        }
    }
    return 0;
}

int do_exit(char** args)
{
    end = 1;
    return 0;
}

int do_quit(char** args)
{
    return do_exit(args);
}

int do_help(char** args)
{
    for(int i = 0; i < CMD_NUM; i++) {
        printf("- %s%s: %s.\n", shell_cmds[i].name, shell_cmds[i].args, shell_cmds[i].help);

    }
    return 0;
}

int do_mount(char** args)
{
    printf("Mounting...\n");
    return 0;
}

int do_lsall(char** args)
{
    printf("Listing files...\n");
    return 0;
}

int do_psb(char** args)
{
    printf("Printing Super Block...\n");
    return 0;
}

int do_cat(char** args)
{
    printf("Reading file...\n");
    return 0;
}

int do_sha(char** args)
{
    printf("Printing SHA of file...\n");
    return 0;
}

int do_inode(char** args)
{
    printf("Printing inode number...\n");
    return 0;
}

int do_istat(char** args)
{
    printf("Printing inode...\n");
    return 0;
}

int do_mkfs(char** args)
{
    printf("Creating new filesystem...\n");
    return 0;
}

int do_mkdir(char** args)
{
    printf("Creating new directory...\n");
    return 0;
}

int do_add(char** args)
{
    printf("Adding new file...\n");
    return 0;
}

int direntv6_dirlookup(const struct unix_filesystem *u, uint16_t inr, const char *entry)
{
    return 0;
}

int tokenize_input(char* input, char** tokenized)
{
    M_REQUIRE_NON_NULL(input); // return error code if NULL
    M_REQUIRE_NON_NULL(tokenized); // return error code if NULL

    tokenized[0] = strtok(input, " "); // compute the first token
    for(int i = 1; i < MAX_ARGS+1; i++) { // compute next tokens
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
    for(int i = 0; i < MAX_ARGS; i++) { // count the number of arguments
        if(tokenized[i+1] != NULL) { // found an valid argument
            argsNb++; // increase number of arguments
        }
    }
    if(argsNb != shell_cmds[index].argc) { // not the expected number of arguments
        return SHELL_INVALID_ARGS; // return appropriate error code
    }
    return shell_cmds[index].fct(&tokenized[1]); // execute command and return its error code

}

void handle_error(int error, char* command)
{
    if(error > 0) { // SHELL ERROR
        printf("ERROR SHELL: %s\n", SHELL_MESSAGES[error - SHELL_FIRST]);
    } else if(error < 0) { // FS error
        printf("ERROR FS: %s", ERR_MESSAGES[error - ERR_FIRST]);
        if(error == ERR_IO) { // IO ERROR
            if(strcmp(command, "add") == 0) { // ERROR occured because of command
                printf(": No such file or directory");
            }
        }
        printf("\n");
    }
}
