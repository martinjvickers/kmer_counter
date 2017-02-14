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

#include <stxxl.h>

using namespace seqan;
using namespace std;

static const int MAX_KEY_LEN = 100;

#define DATA_NODE_BLOCK_SIZE (4096)
#define DATA_LEAF_BLOCK_SIZE (4096)

mutex m; //read query mutex
mutex n; //write to console mutex
mutex o; //count the number of running threads
SeqFileIn queryFileIn;

StringSet<IupacString> queryseqs;
StringSet<CharString> queryids;

//needed for concurrent map
struct MyHashCompare {
    static size_t hash( const string& x ) {
        size_t h = 0;
        for( const char* s = x.c_str(); *s; ++s )
            h = (h*17)^*s;
        return h;
    }
    //! True if strings are equal
    static bool equal( const string& x, const string& y ) {
        return x==y;
    }
};

//until the next comment this is all for the STXXL map which we're not using.
struct CompareGreater
{
	bool operator () (const int & a, const int & b) const
	{
		return a>b;
	}

	static unsigned int max_value()
	{
		return std::numeric_limits<unsigned int>::min();
	}
};

class FixedString { 
public:
    char charStr[MAX_KEY_LEN];

    bool operator< (const FixedString& fixedString) const {
        return std::lexicographical_compare(charStr, charStr+MAX_KEY_LEN,
            fixedString.charStr, fixedString.charStr+MAX_KEY_LEN);
    }

    bool operator==(const FixedString& fixedString) const {
        return std::equal(charStr, charStr+MAX_KEY_LEN, fixedString.charStr);
    }

    bool operator!=(const FixedString& fixedString) const {
        return !std::equal(charStr, charStr+MAX_KEY_LEN, fixedString.charStr);
    } 
};

struct comp_type : public std::less<FixedString> {
    static FixedString max_value()
    {
        FixedString s;
        std::fill(s.charStr, s.charStr+MAX_KEY_LEN, 0x7f);
        return s;
    }

};

std::istream& operator >> (std::istream& i, FixedString& str)
{
	i >> str.charStr;
	return i;
}

std::ostream& operator << (std::ostream& o, FixedString& str)
{
	o << str.charStr;
	return o;
}

//STXXL method
//typedef stxxl::map<FixedString, unsigned int, comp_type, DATA_NODE_BLOCK_SIZE, DATA_LEAF_BLOCK_SIZE> fixed_name_map;
//fixed_name_map myFixedMap((fixed_name_map::node_block_type::raw_size)*5, (fixed_name_map::leaf_block_type::raw_size)*5);


//TBB method which works but uses all your ram!
typedef tbb::concurrent_hash_map<string,int,MyHashCompare> StringTable;
StringTable results;

//User defined options struct
struct ModifyStringOptions
{
        CharString queryFileName = NULL;
        CharString referenceFileName = NULL;
	CharString outputFileName = NULL;
	int klen;
	CharString type;
	int markovOrder;
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

	return seqan::ArgumentParser::PARSE_OK;
}

void count(IupacString sequence, int klen)
{
        int total = 0;
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

void loopcount(int klen, int i, IupacString sequence)
{
	string kmer;
	assign(kmer,infix(sequence, i, i+klen));

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

	StringTable::accessor a;
	results.insert( a, kmer );
	a->second += 1;
}

void applyloop(IupacString sequence, int klen)
{
	int size = length(sequence)-klen;
	tbb::parallel_for(0, size, 1, [=](int i)
	{
		loopcount(klen, i, sequence);
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

	readRecords(queryids, queryseqs, queryFileIn);
	
	int size = length(queryseqs);
	tbb::parallel_for( 0, size , 1, [=](int i)
	{
		applyloop(queryseqs[i], options.klen);
	});

	ofstream outfile;
	outfile.open(toCString(options.outputFileName));

/*
	//This is if you wanted to output the contents of the STXXL map to file.

	fixed_name_map::iterator iter;
	for (iter = myFixedMap.begin(); iter != myFixedMap.end(); ++iter)
	{
		outfile << (FixedString&)iter->first << " " << iter->second << endl;
	}
*/

	for(auto it : results)
		outfile << it.first << " " << it.second << endl;

	outfile.close();

	return 0;
}