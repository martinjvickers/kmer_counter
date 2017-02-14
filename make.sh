###A bit hacky but this allows the make step to fail and then redoes the linking with -ltbb at the end.


#!/bin/sh
make -j4
/usr/bin/c++ -std=c++14 -O3 -DNDEBUG CMakeFiles/kmer_counter.dir/kmer_counter.cpp.o  -o kmer_counter -rdynamic -lrt -lz -Wl,--whole-archive -ltbb -lpthread -Wl,-no-whole-archive
