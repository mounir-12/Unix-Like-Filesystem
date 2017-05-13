LDLIBS += -lcrypto

all: test-inodes test-file test-dirent shell fs test-bitmap test-mount

test-inodes: test-core.o error.o bmblock.o mount.o sector.o inode.o
test-file: test-core.o error.o bmblock.o mount.o sector.o inode.o filev6.o sha.o
test-dirent: test-core.o error.o sector.o bmblock.o mount.o inode.o filev6.o direntv6.o
shell: shell.o error.o sector.o bmblock.o mount.o inode.o filev6.o direntv6.o sha.o
fs.o: fs.c
	$(COMPILE.c) -D_DEFAULT_SOURCE $$(pkg-config fuse --cflags) -o $@ -c $<
fs: fs.o error.o sector.o bmblock.o mount.o inode.o filev6.o direntv6.o
	$(LINK.c) -o $@ $^ $(LDLIBS) $$(pkg-config fuse --libs)
test-bitmap: bmblock.o
test-mount: test-core.o error.o bmblock.o mount.o sector.o inode.o
