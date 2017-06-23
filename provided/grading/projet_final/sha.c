#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>
#include "sha.h"
#include "filev6.h"
#include "inode.h"

static void sha_to_string(const unsigned char *SHA, char *sha_string)
{
    if ((SHA == NULL) || (sha_string == NULL)) {
        return;
    }

    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(&sha_string[i * 2], "%02x", SHA[i]);
    }

    sha_string[2 * SHA256_DIGEST_LENGTH] = '\0';
}

void print_sha_from_content(const unsigned char *content, size_t length)
{
    if(content == NULL) {
        return;
    }
    unsigned char codedData[SHA256_DIGEST_LENGTH]; // array length is length of SHA256
    SHA256(content, length, codedData);

    char sha_string[2 * SHA256_DIGEST_LENGTH + 1]; // array length is length of SHA256 * 2 (+ 1 for '\0')
    sha_to_string(codedData, sha_string);
    printf("%s\n", sha_string); // print SHA256
}


void print_sha_inode(struct unix_filesystem *u, struct inode inode, int inr)
{
    if(u == NULL) {
        return;
    }
    if(inode.i_mode & IALLOC) {
        printf("SHA inode %d: ", inr);
        if(inode.i_mode & IFDIR) {
            printf("no SHA for directories.\n");
        } else {
            struct filev6 fv6;
            int error = filev6_open(u, inr, &fv6);
            if(error) { // error occured
                return; // return
            }

            int32_t size = inode_getsectorsize(&inode);

            unsigned char data[size];

            int read = 0;

            do {
                read = filev6_readblock(&fv6, &(data[fv6.offset]));
                if(read < 0) { // error occured
                    return; // return
                }
            } while(read > 0);
            data[size - 1] = '\0'; // null terminate
            print_sha_from_content(data, inode_getsize(&inode));
        }
    }
}