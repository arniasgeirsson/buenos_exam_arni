rm -f fyams.harddisk
make
cd tests/
make
cd ..

util/tfstool create fyams.harddisk 2048 disk
util/tfstool write fyams.harddisk tests/threads test1
util/tfstool write fyams.harddisk tests/buenos-crew test2
util/tfstool write fyams.harddisk tests/buenos-crew2 test3
util/tfstool write fyams.harddisk tests/task1_test test4
util/tfstool write fyams.harddisk tests/task1_totalsems test5
util/tfstool write fyams.harddisk tests/task1_wrongsem test6
yams buenos 'initprog=[disk]test4' task1_debug

make clean
cd tests/
make clean
cd ..
