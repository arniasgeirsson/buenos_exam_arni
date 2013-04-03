rm -f fyams.harddisk
make
cd tests/
make
cd ..

util/tfstool create fyams.harddisk 2048 disk1
util/tfstool write fyams.harddisk tests/threads test1
util/tfstool write fyams.harddisk tests/buenos-crew test2
yams buenos 'initprog=[disk1]test2' task1_debug

make clean
cd tests/
make clean
cd ..
