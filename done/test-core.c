/**
 * @file test-core.c
 * @brief main program to perform tests on disks (mainly for weeks 04 to 06)
 *
 * @author Aurélien Soccard & Jean-Cédric Chappelier
 * @date 15 Oct 2016
 */

#include <stdlib.h>
#include <stdio.h>
#include "mount.h"
#include "error.h"

#define MIN_ARGS 1
#define MAX_ARGS 1
#define USAGE    "test <diskname>"

int test(struct unix_filesystem *u);
int test_inode_print();
int test_inode_read(const struct unix_filesystem *u, uint16_t inr);
int test_inode_findsector(const struct unix_filesystem *u, uint16_t inr, int32_t file_sec_off);

void error(const char* message)
{
    fputs(message, stderr);
    putc('\n', stderr);
    fputs("Usage: " USAGE, stderr);
    putc('\n', stderr);
    exit(1);
}

void check_args(int argc)
{
    if (argc < MIN_ARGS) {
        error("too few arguments:");
    }
    if (argc > MAX_ARGS) {
        error("too many arguments:");
    }
}

int main(int argc, char *argv[])
{
    // Check the number of args but remove program's name
    check_args(argc - 1);

    struct unix_filesystem u = {0};
    int error = mountv6(argv[1], &u);
    if (error == 0) {
        printf("\nmountv6_print_superblock test: \n\n");
        mountv6_print_superblock(&u);
        error = test(&u);
        
        printf("\ninode_print test: \n\n");
        test_inode_print();
        
        uint16_t inr = 5;
        printf("\ninode_read test on inode %u: \n\n", inr);
        test_inode_read(&u,inr);
        
        int32_t file_sec_off = 8;
        printf("\ninode_findsector test on inode %u and offset %u: \n\n", inr, file_sec_off);
        test_inode_findsector(&u,inr, file_sec_off);
    }
    if (error) {
        puts(ERR_MESSAGES[error - ERR_FIRST]);
    }
    umountv6(&u); /* shall umount even if mount failed,
                   * for instance fopen could have succeeded
                   * in mount (thus fclose required).
                   */

    return error;
}
