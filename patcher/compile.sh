cd /usr/src/minix/servers/vfs/
make clean && make
sleep 1
make install
cd /usr/src/releasetools
make do-hdboot
