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


#####Prefix tests#####

```
mvickers@n108379:~/development/kmer_counter$ /usr/bin/time -v ./kmer_counter -q example_data/SRR042642_1million.fastq.gz -k 55 -o /dev/shm/meh.dat -p 1
	Command being timed: "./kmer_counter -q example_data/SRR042642_1million.fastq.gz -k 55 -o /dev/shm/meh.dat -p 1"
	User time (seconds): 61.67
	System time (seconds): 8.89
	Percent of CPU this job got: 139%
	Elapsed (wall clock) time (h:mm:ss or m:ss): 0:50.74
	Average shared text size (kbytes): 0
	Average unshared data size (kbytes): 0
	Average stack size (kbytes): 0
	Average total size (kbytes): 0
	Maximum resident set size (kbytes): 4261208
	Average resident set size (kbytes): 0
	Major (requiring I/O) page faults: 0
	Minor (reclaiming a frame) page faults: 1243484
	Voluntary context switches: 97949
	Involuntary context switches: 28842
	Swaps: 0
	File system inputs: 0
	File system outputs: 0
	Socket messages sent: 0
	Socket messages received: 0
	Signals delivered: 0
	Page size (bytes): 4096
	Exit status: 0
```
```
mvickers@n108379:~/development/kmer_counter$ /usr/bin/time -v ./kmer_counter -q example_data/SRR042642_1million.fastq.gz -k 55 -o /dev/shm/meh.dat -p 4
	Command being timed: "./kmer_counter -q example_data/SRR042642_1million.fastq.gz -k 55 -o /dev/shm/meh.dat -p 4"
	User time (seconds): 63.36
	System time (seconds): 7.19
	Percent of CPU this job got: 143%
	Elapsed (wall clock) time (h:mm:ss or m:ss): 0:49.26
	Average shared text size (kbytes): 0
	Average unshared data size (kbytes): 0
	Average stack size (kbytes): 0
	Average total size (kbytes): 0
	Maximum resident set size (kbytes): 3589224
	Average resident set size (kbytes): 0
	Major (requiring I/O) page faults: 2
	Minor (reclaiming a frame) page faults: 887719
	Voluntary context switches: 93577
	Involuntary context switches: 29800
	Swaps: 0
	File system inputs: 208
	File system outputs: 0
	Socket messages sent: 0
	Socket messages received: 0
	Signals delivered: 0
	Page size (bytes): 4096
	Exit status: 0
```

No prefix and doing the read file in batches

```
Processing batch 980000 990000 size 1 1 0 19323074
Processing batch 990000 1000000 size 1 1 0 19516241
	Command being timed: "./kmer_counter -q example_data/SRR042642_1million.fastq.gz -k 55 -o /dev/shm/meh.dat"
	User time (seconds): 1075.41
	System time (seconds): 9.05
	Percent of CPU this job got: 332%
	Elapsed (wall clock) time (h:mm:ss or m:ss): 5:26.07
	Average shared text size (kbytes): 0
	Average unshared data size (kbytes): 0
	Average stack size (kbytes): 0
	Average total size (kbytes): 0
	Maximum resident set size (kbytes): 3334520
	Average resident set size (kbytes): 0
	Major (requiring I/O) page faults: 0
	Minor (reclaiming a frame) page faults: 852882
	Voluntary context switches: 14439
	Involuntary context switches: 661370
	Swaps: 0
	File system inputs: 0
	File system outputs: 0
	Socket messages sent: 0
	Socket messages received: 0
	Signals delivered: 0
	Page size (bytes): 4096
	Exit status: 0
```

This time, two prefixes of 4 and 4. All data read in with seqan

```
Processing batch 0 10000 size 1 1 0 0
	Command being timed: "./kmer_counter -q example_data/SRR042642_1million.fastq.gz -k 55 -o /dev/shm/meh.dat -p 4"
	User time (seconds): 76.30
	System time (seconds): 7.79
	Percent of CPU this job got: 156%
	Elapsed (wall clock) time (h:mm:ss or m:ss): 0:53.79
	Average shared text size (kbytes): 0
	Average unshared data size (kbytes): 0
	Average stack size (kbytes): 0
	Average total size (kbytes): 0
	Maximum resident set size (kbytes): 3656104
	Average resident set size (kbytes): 0
	Major (requiring I/O) page faults: 0
	Minor (reclaiming a frame) page faults: 926896
	Voluntary context switches: 92279
	Involuntary context switches: 35544
	Swaps: 0
	File system inputs: 0
	File system outputs: 0
	Socket messages sent: 0
	Socket messages received: 0
	Signals delivered: 0
	Page size (bytes): 4096
	Exit status: 0
```

No prefix but all data read in with seqan
```
mvickers@n108379:~/development/kmer_counter$ /usr/bin/time -v ./kmer_counter -q example_data/SRR042642_1million.fastq.gz -k 55 -o /dev/shm/meh.dat -p 4
Processing batch 0 10000 size 1 1 0 0
	Command being timed: "./kmer_counter -q example_data/SRR042642_1million.fastq.gz -k 55 -o /dev/shm/meh.dat -p 4"
	User time (seconds): 55.96
	System time (seconds): 8.30
	Percent of CPU this job got: 153%
	Elapsed (wall clock) time (h:mm:ss or m:ss): 0:41.93
	Average shared text size (kbytes): 0
	Average unshared data size (kbytes): 0
	Average stack size (kbytes): 0
	Average total size (kbytes): 0
	Maximum resident set size (kbytes): 3589168
	Average resident set size (kbytes): 0
	Major (requiring I/O) page faults: 0
	Minor (reclaiming a frame) page faults: 900242
	Voluntary context switches: 91848
	Involuntary context switches: 27353
	Swaps: 0
	File system inputs: 0
	File system outputs: 0
	Socket messages sent: 0
	Socket messages received: 0
	Signals delivered: 0
	Page size (bytes): 4096
	Exit status: 0
```
