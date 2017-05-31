LDLIBS += -lcrypto

all: test-inodes test-file test-dirent shell

test-inodes: test-core.o error.o mount.o sector.o inode.o
test-file: test-core.o error.o mount.o sector.o inode.o filev6.o sha.o
test-dirent: test-core.o error.o sector.o mount.o inode.o filev6.o direntv6.o
shell: shell.o error.o sector.o mount.o inode.o filev6.o direntv6.o sha.o
