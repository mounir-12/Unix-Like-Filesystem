#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>
#include "sha.h"

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
    unsigned char codedData[SHA_DIGEST_LENGTH];
    unsigned char * sha = SHA256(content, length, codedData);

    char sha_string[SHA_DIGEST_LENGTH];
    sha_to_string(sha, sha_string);
}


void print_sha_inode(struct unix_filesystem *u, struct inode inode, int inr)
{
	if(inode.i_mode & IALLOC) {
        printf("SHA inode %d: ", inr);
        if(inode.i_mode & IFDIR){
			printf("no SHA for directories.\n");
		} else {
			printf("content %d SHA\n", inr);
		}
    }
}
