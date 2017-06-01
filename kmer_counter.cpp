#include <iostream>
#include <seqan/sequence.h>  // CharString, ...
#include <seqan/stream.h>    // to stream a CharString into cout
#include <seqan/file.h>
#include <seqan/arg_parse.h>
#include <seqan/seq_io.h>
#include <queue>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <limits>
#include <ctime>

#include "tbb/tbb.h"
#include "tbb/concurrent_hash_map.h"

#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "tbb/tick_count.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/tbb_allocator.h"

using namespace seqan;
using namespace std;

mutex m; //read query mutex
mutex n; //write to console mutex
mutex o; //count the number of running threads
SeqFileIn queryFileIn;

StringSet<IupacString> queryseqs;
StringSet<CharString> queryids;

struct MyHashCompare {
    static size_t hash( const Dna5String& x ) {
        size_t h = 0;
	for(int i = 0; i < length(x); i++)
		h =(h*17)^(char)x[i];
        return h;
    }
    //! True if strings are equal
    static bool equal( const Dna5String& x, const Dna5String& y ) {
        return x==y;
    }
};

//TBB method which works but uses all your ram!
typedef tbb::concurrent_hash_map<Dna5String,int,MyHashCompare> StringTable;
StringTable results;

map<string, map<string, tbb::concurrent_hash_map<string,int,MyHashCompare>>> more;

//User defined options struct
struct ModifyStringOptions
{
        CharString queryFileName = NULL;
        CharString referenceFileName = NULL;
	CharString outputFileName = NULL;
	int klen;
	CharString type;
	int markovOrder;
	int prefix;
};

//Parse our commandline options
seqan::ArgumentParser::ParseResult parseCommandLine(ModifyStringOptions & options, int argc, char const ** argv)
{
	seqan::ArgumentParser parser("kmer_counter");
	addOption(parser, seqan::ArgParseOption("q", "query-file", "Path to the file containing your query sequence data.\n", seqan::ArgParseArgument::INPUT_FILE, "IN"));
	setValidValues(parser, "query-file", toCString(concat(getFileExtensions(SeqFileIn()), ' ')));
	addOption(parser, seqan::ArgParseOption("o", "output-file", "Output file.", seqan::ArgParseArgument::OUTPUT_FILE, "OUT"));
	setRequired(parser, "output-file");
	addOption(parser, seqan::ArgParseOption("k", "klen", "K-Mer Size.", seqan::ArgParseArgument::INTEGER, "INT"));
        setDefaultValue(parser, "klen", "3");
	addOption(parser, seqan::ArgParseOption("p", "prefix", "Prefix Size.", seqan::ArgParseArgument::INTEGER, "INT"));
        setDefaultValue(parser, "prefix", "3");
	setShortDescription(parser, "Alignment-free sequence comparison.");
	setVersion(parser, "0.0.1");
	setDate(parser, "February 2017");
	addUsageLine(parser, "-q query.fasta -k 9 -o results.txt [\\fIOPTIONS\\fP] ");
	addDescription(parser, "K-Mer counting using TBB for concurrent map and parallel loops");

	seqan::ArgumentParser::ParseResult res = seqan::parse(parser, argc, argv);

	// Only extract options if the program will continue after parseCommandLine()
	if (res != seqan::ArgumentParser::PARSE_OK)
		return res;

	//begin extracting options
	getOptionValue(options.queryFileName, parser, "query-file");
	getOptionValue(options.outputFileName, parser, "output-file");
	getOptionValue(options.klen, parser, "klen");
	getOptionValue(options.prefix, parser, "prefix");

	return seqan::ArgumentParser::PARSE_OK;
}

void count(IupacString sequence, int klen)
{
        //int total = 0;
        //iterate over the sequence
        for(int i = 0; i <= length(sequence)-klen; i++)
        {
                //get our kmer
                string kmer;
                assign(kmer,infix(sequence, i, i+klen));

                //need to drop if there is an N in it
                size_t found = kmer.find("N");

                if(found > kmer.size()){
			StringTable::accessor a;
			results.insert(a, kmer);
			a->second += 1;
                }
        }
}

void loopcount(int klen, int i, IupacString sequence, int prefixsize, int prefix2size)
{
	string kmer;
	//assign(kmer,infix(sequence, i+prefixsize+prefix2size, i+klen));
	assign(kmer,infix(sequence, i, i+klen));
/*
	string prefix;
	assign(prefix,infix(sequence, i, i+prefixsize));

	string prefix2;
        assign(prefix2,infix(sequence, i+prefixsize, i+prefixsize+prefix2size));
*/
	//m.lock();
	//cout << prefix << " " << prefix2 << " " << kmer<< endl;
	//m.unlock();

	//map/map/concurrent map example
	//StringTable::accessor a;
	//more[prefix][prefix2].insert(a,kmer);
	//a->second += 1;

	//this allows us to make the FixedString from a string
	//FixedString fs;
	//strncpy(fs.charStr, kmer.c_str(), MAX_KEY_LEN-1);
	//for(int i = kmer.length(); i < MAX_KEY_LEN-1; i++)
        //	fs.charStr[i] = ' ';
	//fs.charStr[MAX_KEY_LEN-1] = '\0';

	//if you wanted to use STXXL, this is how you'd increment the STXXL map
	//m.lock();
	//myFixedMap[fs]++;
	//m.unlock();

	size_t found = kmer.find("N");
	if(found > kmer.size()){
		StringTable::accessor a;
		results.insert( a, kmer );
		a->second += 1;
	}
}

void applyloop(IupacString &sequence, int klen, int prefixsize, int prefix2size)
{
	int size = length(sequence)-klen;
	tbb::parallel_for(0, size, 1, [=](int i)
	{
		loopcount(klen, i, sequence, prefixsize, prefix2size);
	});
}

int main(int argc, char const ** argv)
{
	//parse our options
	ModifyStringOptions options;
	seqan::ArgumentParser::ParseResult res = parseCommandLine(options, argc, argv);

	// If parsing was not successful then exit with code 1 if there were errors.
	// Otherwise, exit with code 0 (e.g. help was printed).
	if (res != seqan::ArgumentParser::PARSE_OK)
		return res == seqan::ArgumentParser::PARSE_ERROR;

	//open query file for the threads to read in batches
	if(!open(queryFileIn, (toCString(options.queryFileName))))
	{
		cerr << "Error: could not open file " << toCString(options.queryFileName) << endl;
		return 1;
	}

	int counter = 0;

	while(!atEnd(queryFileIn))
	{
		readRecords(queryids, queryseqs, queryFileIn);
	
		int size = length(queryseqs);
		tbb::parallel_for( 0, size , 1, [=](int i)
		{
			//applyloop(queryseqs[i], options.klen, options.prefix, 4);
			count(queryseqs[i], options.klen);
		});
	}

	ofstream outfile;
        outfile.open(toCString(options.outputFileName), ios::binary);

	for(auto it : results)
		outfile << it.first << " " << it.second << endl;

	outfile.close();

	return 0;
}
