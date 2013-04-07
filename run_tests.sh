rm -f fyams.harddisk
make
cd tests/
make
cd ..

util/tfstool create fyams.harddisk 2048 disk

util/tfstool write fyams.harddisk tests/threads test1
util/tfstool write fyams.harddisk tests/buenos-crew test2
util/tfstool write fyams.harddisk tests/buenos-crew2 test3

util/tfstool write fyams.harddisk tests/task1_test task1_test
util/tfstool write fyams.harddisk tests/task1_totalsems totalsems
util/tfstool write fyams.harddisk tests/task1_wrongsem wrongsem
util/tfstool write fyams.harddisk tests/task2_test task2_test
#yams buenos 'initprog=[disk]task1_test' task1_debug
yams buenos 'initprog=[disk]task2_test' task1_debug

make clean
cd tests/
make clean
cd ..
