/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall hello.c `pkg-config fuse --cflags --libs` -o hello
*/

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include "error.h"
#include "direntv6.h"
#include "inode.h"
#include "filev6.h"

/* From https://github.com/libfuse/libfuse/wiki/Option-Parsing.
 * This will look up into the args to search for the name of the FS.
 */
static int arg_parse(void *data, const char *filename, int key, struct fuse_args *outargs);

struct unix_filesystem fs;

static int fs_getattr(const char *path, struct stat *stbuf)
{
    memset(stbuf, 0, sizeof(struct stat));

    int inr = direntv6_dirlookup(&fs, ROOT_INUMBER, path); // search inode number
    if(inr < 0) { // inode not found
        return inr; // propagate error code
    }
    
    struct inode i;
    int error = inode_read(&fs, inr, &i); // read inode
    if(error) { // error found
        return error; // propagate error
    }

    stbuf->st_dev = 0;
    stbuf->st_ino = inr;
    if (i.i_mode & IFDIR) { // inode is a directory
        stbuf->st_mode = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    } else { // inode is a file
        stbuf->st_mode = S_IFREG | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    }
    stbuf->st_nlink = 0;
    stbuf->st_uid = i.i_uid;
    stbuf->st_gid = i.i_gid;
    stbuf->st_rdev = 0;
    stbuf->st_size = inode_getsize(&i);
    stbuf->st_blksize = 0;
    stbuf->st_blocks = 0;
    stbuf->st_atime = i.i_atime[0];
    stbuf->st_mtime = i.i_mtime[0];
    stbuf->st_ctime = 0;

    error = direntv6_print_tree(&fs, ROOT_INUMBER, "");
    if(error) { // error found
        return error; //propagate error
    }

    return 0;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    int inr = direntv6_dirlookup(&fs, ROOT_INUMBER, path); // search inode number
    if(inr < 0) { // inode not found
        return inr; // propagate error code
    }
    struct directory_reader d;
    int openingError = direntv6_opendir(&fs, inr, &d); // error occured while opening the directory
    if(openingError) { // propagate error
        return openingError;
    }
    int read = 0;

    do {
        char childName[DIRENT_MAXLEN+1];
        uint16_t child_inr;
        read = direntv6_readdir(&d, childName, &child_inr); // read directory's children
        if(read < 0){
			return read;
		}
		filler(buf, childName, NULL, 0); // copy child's name in the buffer
    } while(read == 1); // read until there are no more childrens left

    return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi)
{
    (void) fi;
    
    int inr = direntv6_dirlookup(&fs, ROOT_INUMBER, path); // search inode number
    if(inr < 0) { // inode not found
        return inr; // propagate error code
    }
    struct filev6 fv6;
    filev6_open(&fs, inr, &fv6);
    
    int offsetError = filev6_lseek(&fv6, offset);
    if(offsetError){
		return offsetError;
	}
	
	struct inode i;
    int error = inode_read(&fs, inr, &i); // read inode
    if(error) { // error found
        return error; // propagate error
    }
    
    uint32_t i_size = inode_getsize(&i);
    memcpy(buf, &i, sizeof(struct inode));

    
    return i_size;
}

static struct fuse_operations available_ops = {
    .getattr	= fs_getattr,
    .readdir	= fs_readdir,
    .read		= fs_read,
};

static int arg_parse(void *data, const char *filename, int key, struct fuse_args *outargs)
{
    (void) data;
    (void) outargs;
    if (key == FUSE_OPT_KEY_NONOPT && fs.f == NULL && filename != NULL) {
        int error = mountv6(filename,&fs);
        if(error) {
			printf("ERROR FS: %s", ERR_MESSAGES[error - ERR_FIRST]);
            fs.f = NULL;
            exit(1);
        }
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[])
{
	fs.f = NULL; // initial value of f
	// main
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    int ret = fuse_opt_parse(&args, NULL, NULL, arg_parse);
    if (ret == 0) {
        ret = fuse_main(args.argc, args.argv, &available_ops, NULL);
        (void)umountv6(&fs);
    }
    return ret;
}
