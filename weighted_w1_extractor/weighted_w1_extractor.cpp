#include <iostream>
#include <seqan/sequence.h>  // CharString, ...
#include <seqan/stream.h>    // to stream a CharString into cout
#include <seqan/file.h>
#include <seqan/arg_parse.h>
#include <seqan/bam_io.h>
#include <seqan/gff_io.h>
#include <string>
using namespace seqan;
using namespace std;

struct ModifyStringOptions
{
        CharString inputFileName;
	CharString outputFileName;
	CharString label;
};

seqan::ArgumentParser::ParseResult parseCommandLine(ModifyStringOptions & options, int argc, char const ** argv)
{
        seqan::ArgumentParser parser("weighted_w1_extractor");
        setShortDescription(parser, "XFTOOLS");
        setVersion(parser, "0.0.6");
        setDate(parser, "November 2017");
        addUsageLine(parser, "-i input.bam -o output.gff [\\fIOPTIONS\\fP] ");
	addDescription(parser, "The purpose of this program is to create a GFF file containing the (weighted) number of times a base is covered by a read. A GFF which contains information about each base in our lab is known as a w1 file (AKA window of size 1). A regular w1 file is would simply count the number of times a base is covered by a read, if this is the functionality you desire, then use the bam_2_w1_extractor program. The weighted part of this is that if your alignment file contains reads which have mapped multiple times, e.g. for when looking at sRNA using bowtie2 searching and reporting all results, you may have a read that has mapped in several locations. If you have a read that maps to three locations, rather than counting that as +1 to each location, it will be counted as +(1/3) to each location. ");

	// accept an input file
	addOption(parser, seqan::ArgParseOption("i", "input-file", "Path to the input file", seqan::ArgParseArgument::INPUT_FILE, "IN"));
        setRequired(parser, "input-file");
	addOption(parser, seqan::ArgParseOption("o", "output-file", "Path to the output file", seqan::ArgParseArgument::OUTPUT_FILE, "OUT"));
	setRequired(parser, "output-file");
	addOption(parser, seqan::ArgParseOption("l", "label", "Column 3 GFF output label. Useful if using SignalMap as GFFs with the same label will be merged.", seqan::ArgParseArgument::STRING, "TEXT"));
	setDefaultValue(parser, "label", "window");

        seqan::ArgumentParser::ParseResult res = seqan::parse(parser, argc, argv);

        // If parsing was not successful then exit with code 1 if there were errors.
        // Otherwise, exit with code 0 (e.g. help was printed).
        if (res != seqan::ArgumentParser::PARSE_OK)
                return res;

	// parse the inputs into the options struct
        getOptionValue(options.inputFileName, parser, "input-file");
	getOptionValue(options.outputFileName, parser, "output-file");
	getOptionValue(options.label, parser, "label");

        return seqan::ArgumentParser::PARSE_OK;
}

/*
	Stores the counts for each base
*/
typedef std::map<int, double> chrmap;
typedef std::map<int, chrmap> completemap;

/*
	Write out the counts to a GFF file
*/
void writeToFile(completemap &counter, BamFileIn &inFile, GffFileOut &gffOutFile, ModifyStringOptions options)
{

	for(auto i : counter)
	{
		for(auto j : i.second)
		{
			typedef FormattedFileContext<BamFileIn, void>::Type TBamContext;
			TBamContext const & bamContext = context(inFile);

			GffRecord record;
			record.ref = contigNames(bamContext)[i.first];
			record.source = "xftools";
			record.type = options.label;
			record.beginPos = j.first;
			record.endPos = j.first;
			record.strand = '.';
			record.score = (double)j.second;
			writeRecord(gffOutFile, record);

		}
	}
}

int main(int argc, char const ** argv)
{
	ModifyStringOptions options;
        seqan::ArgumentParser::ParseResult res = parseCommandLine(options, argc, argv);
	
	// If parsing was not successful then exit with code 1 if there were errors.
	// Otherwise, exit with code 0 (e.g. help was printed).
	if (res != seqan::ArgumentParser::PARSE_OK)
		return res == seqan::ArgumentParser::PARSE_ERROR;

	// read in input bam file
	BamFileIn inFile;
	if (!open(inFile, toCString(options.inputFileName)))
	{
		std::cerr << "ERROR: Could not open " << options.inputFileName << " for reading.\n";
		return 1;
	}

	// create output file
	GffFileOut gffOutFile;
	if(!open(gffOutFile, toCString(options.outputFileName)))
	{
		std::cerr << "ERROR: Could not open output.gff" " for reading.\n";
		return 1;
	}

	completemap base_counter;
	vector<BamAlignmentRecord> mapping_results; // stores all the mapping records for the same read

	BamHeader header;
	readHeader(header, inFile);
	BamAlignmentRecord record;
	int rID = -1;
	CharString qName;

	do
	{
		readRecord(record, inFile);

		// if first time
		if(rID < 0)
		{
			rID = record.rID;
			qName = record.qName;
		}

		// if the current record ID does not match the last one then we know it's a new read
		if(record.qName != qName)
		{
			// go through each record in vector
			for(auto rec : mapping_results)
			{
				// go through each pos of the record
				for(int i = rec.beginPos; i < (rec.beginPos + length(record.seq)); i++)
				{
					// calculate the weighted count
					double tmp = base_counter[rec.rID][i];
					tmp = (double)tmp + ((double)1.0/(double)mapping_results.size());
					base_counter[rec.rID][i] = tmp;
				}
			}
			mapping_results.clear(); // clear vector
			rID = record.rID;
			qName = record.qName;
		}

		// add currect record to current vector
		mapping_results.push_back(record);
		
	} while(!atEnd(inFile));

	writeToFile(base_counter, inFile, gffOutFile, options);

	close(inFile);
	close(gffOutFile);

	return 0;
}
