#include <iostream>
#include <seqan/sequence.h>  // CharString, ...
#include <seqan/stream.h>    // to stream a CharString into cout
#include <seqan/file.h>
#include <seqan/arg_parse.h>
#include <seqan/gff_io.h>
#include <seqan/seq_io.h>
#include <string>
using namespace seqan;
using namespace std;

struct ModifyStringOptions
{
        CharString inputFileName;
	CharString outputFileName;
	CharString label;
	int window_size;
};

seqan::ArgumentParser::ParseResult parseCommandLine(ModifyStringOptions & options, int argc, char const ** argv)
{
        seqan::ArgumentParser parser("cytosine_abundance");
        setShortDescription(parser, "XFTOOLS");
        setVersion(parser, "0.0.1");
        setDate(parser, "October 2017");
        addUsageLine(parser, "-i input.fa -o output.gff [\\fIOPTIONS\\fP] ");
	addDescription(parser, "The purpose of this program is to calculate the cytosine abundance and cytosine context's in a reference genome.");

	// accept an input file
	addOption(parser, seqan::ArgParseOption("i", "input-file", "Path to the input file", seqan::ArgParseArgument::INPUT_FILE, "IN"));
        setRequired(parser, "input-file");
	addOption(parser, seqan::ArgParseOption("o", "output-file", "Path to the output file", seqan::ArgParseArgument::OUTPUT_FILE, "OUT"));
	setRequired(parser, "output-file");
	addOption(parser, seqan::ArgParseOption("l", "label", "Column 3 GFF output label. Useful if using SignalMap as GFFs with the same label will be merged.", seqan::ArgParseArgument::STRING, "TEXT"));
	addOption(parser, seqan::ArgParseOption("s", "window-size", "Size of window",seqan::ArgParseArgument::INTEGER, "INT"));
	setDefaultValue(parser, "window-size", "50");
	
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
	getOptionValue(options.window_size, parser, "window-size");

        return seqan::ArgumentParser::PARSE_OK;
}

int main(int argc, char const ** argv)
{
	ModifyStringOptions options;
        seqan::ArgumentParser::ParseResult res = parseCommandLine(options, argc, argv);
	
	// If parsing was not successful then exit with code 1 if there were errors.
	// Otherwise, exit with code 0 (e.g. help was printed).
	if (res != seqan::ArgumentParser::PARSE_OK)
		return res == seqan::ArgumentParser::PARSE_ERROR;

	// create output file
	GffFileOut gffOutFile;
	if(!open(gffOutFile, toCString(options.outputFileName)))
	{
		std::cerr << "ERROR: Could not open output.gff" " for reading.\n";
		return 1;
	}

	//index the reference
	FaiIndex faiIndex;
	if (!build(faiIndex, toCString(options.inputFileName)))
    	{
        	std::cerr << "ERROR: Could not build FAI index for file " << options.inputFileName << ".\n";
        	return 0;
    	} else {
		cout << "Index created of reference file. " << endl;
	}

	// method.
	// create index of reference
	// loop through the reference in windows
		// count c's + contexts
		// reverse compliment and repeat
		// print to GFF

	close(gffOutFile);

	return 0;
}
