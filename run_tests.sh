rm -f fyams.harddisk
make
cd tests/
make
cd ..

util/tfstool create fyams.harddisk 2048 disk
util/tfstool write fyams.harddisk tests/buenos-crew.c bcrew
util/tfstool write fyams.harddisk tests/my_crew_solution.c mycrew
util/tfstool write fyams.harddisk tests/task1_test task1_test
util/tfstool write fyams.harddisk tests/task1_totalsems totalsems
util/tfstool write fyams.harddisk tests/task2_test task2_test
yams buenos 'initprog=[disk]task1_test'
yams buenos 'initprog=[disk]task2_test'

make clean
cd tests/
make clean
cd ..
