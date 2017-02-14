### Quick summary ###
Simple concurrent K-Mer counter using TBB (and STXXL but not really used in the example as it's not concurrent).

### How do I get set up on Ubuntu 16.04? ###

```
sudo apt-get install git g++ build-essential cmake zlib1g-dev libbz2-dev libboost-all-dev libtbb-dev libstxxl-dev
git clone https://github.com/seqan/seqan.git seqan
git clone https://github.com/martinjvickers/kmer_counter.git
cd alfsc
cmake ../kmer_counter -DCMAKE_MODULE_PATH=../seqan/util/cmake -DSEQAN_INCLUDE_PATH=../seqan/include -DCMAKE_CXX_FLAGS=-std=c++14 -DCMAKE_BUILD_TYPE=Release
./make.sh
```

Don't worry about the initial tbb errors. The ./make.sh script reruns the linker after the regular make -j4 command which produces the errors. Once I figure out how to append ltbb to the end of the make file this will disappear.

### Quick run test ###

This took just under 4 mins and 40GB of ram on an AWS 32core 240GB Ubuntu 16.4 instance to calculate k=55 for 1GB compressed fastq file with approx 16million reads.

```
ubuntu@ip-172-31-6-42:~$ /usr/bin/time -v ./kmer_counter -q SRR042642_1.fastq.gz -k 55 -o /dev/shm/meh.dat
	Command being timed: "./kmer_counter -c 1 -q SRR042642_1.fastq.gz -k 55 -o /dev/shm/meh.da"
	User time (seconds): 991.66
	System time (seconds): 105.51
	Percent of CPU this job got: 501%
	Elapsed (wall clock) time (h:mm:ss or m:ss): 3:38.71
	Average shared text size (kbytes): 0
	Average unshared data size (kbytes): 0
	Average stack size (kbytes): 0
	Average total size (kbytes): 0
	Maximum resident set size (kbytes): 39746760
	Average resident set size (kbytes): 0
	Major (requiring I/O) page faults: 0
	Minor (reclaiming a frame) page faults: 7117140
	Voluntary context switches: 9057648
	Involuntary context switches: 5855
	Swaps: 0
	File system inputs: 0
	File system outputs: 0
	Socket messages sent: 0
	Socket messages received: 0
	Signals delivered: 0
	Page size (bytes): 4096
	Exit status: 0
```
