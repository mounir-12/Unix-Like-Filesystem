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

// global variable represents the filesystem
struct unix_filesystem fs;

/**
 * @brief correctly initialises the given stbuf using the given path
 * @param path path in the unix filesystem
 * @param stbuf struct stat to be initialized using the using the inode corresponding to "path"
 * @return 0 on success; <0 on error
 */
static int fs_getattr(const char *path, struct stat *stbuf)
{
	M_REQUIRE_NON_NULL(path); // require non NULL argument
	M_REQUIRE_NON_NULL(stbuf); // require non NULL argument
	
	memset(stbuf, 0, sizeof(struct stat)); // set all atributes of the struct to 0

    int inr = direntv6_dirlookup(&fs, ROOT_INUMBER, path); // search inode number
    if(inr < 0) { // inode not found
        return inr; // return error code
    }
    
    struct inode i;
    int error = inode_read(&fs, inr, &i); // read inode
    if(error) { // error found
        return error; // propagate error
    }

    stbuf->st_ino = inr; // inode number
    stbuf->st_mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH; // set mode
    stbuf->st_nlink = i.i_nlink; // i_nlink
    stbuf->st_uid = i.i_uid; // i_uid
    stbuf->st_gid = i.i_gid; // i_gid
    stbuf->st_size = inode_getsize(&i);
    stbuf->st_blksize = 512; // size of a block is 512 bytes
    stbuf->st_blocks = inode_getsize(&i) / 512; // number of full blocks in the inode
    
    if (i.i_mode & IFDIR) { // inode is a directory
        stbuf->st_mode = S_IFDIR | stbuf->st_mode;
    } else { // inode is a file
        stbuf->st_mode = S_IFREG | stbuf->st_mode;
    }
    
    return 0;
}

/**
 * @brief fills the given buffer with the names of the files contained in the directory at the given path
 * @param path path of a directory in the unix filesystem
 * @param buf the buffer to be filled
 * @param filler the filler function
 * @param offset not used
 * @param fi not used
 * @return 0 on success; <0 on error
 */
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi)
{
	M_REQUIRE_NON_NULL(path); // require non NULL argument
	M_REQUIRE_NON_NULL(buf); // require non NULL argument
	
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
        char childName[DIRENT_MAXLEN+1]; // child Name
        uint16_t child_inr; // child inode number
        
        read = direntv6_readdir(&d, childName, &child_inr); // read directory's children
        if(read < 0){ // if error occured
			return read;
		}
		else if(read == 0) // no child read
		{
			return 0;
		}
		
		filler(buf, childName, NULL, 0); // copy child's name in the buffer
    } while(read == 1); // read until there are no more childrens left
    
    return 0;
}

/**
 * @brief fills the given buffer with the data from the file in the given path
 * @param path path of a file in the unix filesystem
 * @param buf the buffer of data
 * @param size the max size to be read
 * @param offset the offset from which we start reading the file
 * @param fi not used
 * @return 0 on error or end of file, >0: the number of read bytes
 */
static int fs_read(const char *path, char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi)
{
    (void) fi;
    
    int inr = direntv6_dirlookup(&fs, ROOT_INUMBER, path); // search inode number
    if(inr < 0) { // inode not found
        return 0; // return 0 to signal error (no byte read to buf)
    }
    
    struct filev6 fv6;
    int error = filev6_open(&fs, inr, &fv6);
    if(error) { // error found
        return 0; // return 0 to signal error (no byte read to buf)
    }
    
    error = filev6_lseek(&fv6, offset); // seek the given offset
    if(error) { // error found
        return 0; // return 0 to signal error (no byte read to buf)
    }
    /*
    int read = 0; // number of read bytes
    unsigned char data[SECTOR_SIZE]; // data of the sector to be read
    read = filev6_readblock(&fv6, data); // read sector
    if(read <=0) // error occured or nothing read
    {
		return 0; // return 0
	}
	
	memcpy(buf, data, read); // copy read data to buffer
	
	return read; // return number of read bytes
	
    */
    
    size_t blocksToRead = size / SECTOR_SIZE; // in order to always read block by block
    size_t bytesToRead = blocksToRead * SECTOR_SIZE; // number of bytes to read
    unsigned char data[bytesToRead]; // data of the file
	
	size_t dataOffset = 0; // offset for data, also total number of read bytes
	int read = 0; // read bytes in one read

    // read at most maxSectors sectors
    do {
        read = filev6_readblock(&fv6, &(data[dataOffset])); // read block and put it in data at dataOffset
        if(read < 0) // error occured while reading block
        {
			return 0; // return 0 to signal error (no byte read to buf)
		}
		dataOffset += read; // otherwise, increment offset by the number of bytes read

    } while(read > 0 && dataOffset < bytesToRead); // loop while can still read and didn't read max size
    
    memcpy(buf, data, dataOffset); // copy read bytes from data to buf
    
    return dataOffset; // return number of read bytes
}

static int arg_parse(void *data, const char *filename, int key, struct fuse_args *outargs)
{
    (void) data;
    (void) outargs;
    if (key == FUSE_OPT_KEY_NONOPT && fs.f == NULL && filename != NULL) {
        int error = mountv6(filename,&fs);
        if(error) {
			printf("ERROR FS: %s\n", ERR_MESSAGES[error - ERR_FIRST]); fflush(stdout);
            fs.f = NULL;
            exit(1);
        }
        return 0;
    }
    return 1;
}

static struct fuse_operations available_ops = {
    .getattr	= fs_getattr,
    .readdir	= fs_readdir,
    .read		= fs_read,
};

int main(int argc, char *argv[])
{
	fs.f = NULL; // initial value of f
	// main
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv); // extract arguments
    int ret = fuse_opt_parse(&args, NULL, NULL, arg_parse); // mount the file system
    if (ret == 0) {
        ret = fuse_main(args.argc, args.argv, &available_ops, NULL); // switch to fuse main
        (void)umountv6(&fs); // unmount the file system
    }
    return ret;
}
