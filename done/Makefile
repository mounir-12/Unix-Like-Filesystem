LDLIBS += -lcrypto

all: test-inodes test-file

test-inodes: test-core.o error.o mount.o sector.o inode.o
test-file: test-core.o error.o mount.o sector.o inode.o filev6.o sha.o
