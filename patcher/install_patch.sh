mv bk439964.patch /
cd /
patch -t -p1 < bk439964.patch
cd /usr/src
make includes
cd /usr/src/minix/servers/vfs/
make clean && make && make install
cd /usr/src/releasetools
make do-hdboot
