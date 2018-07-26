// Dumper - A bare-boned command line dumper programmed using Klarth's TableLib
// Dumps via offsets rather than pointer tables

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>
#include "TextDecoder.h"

using namespace std;

string EndString = "<END>\n\n"; // Change this if your endline value is different

int main(int argc, char* argv[])
{
	TextDecoder Dumper;
	unsigned int StartOffset = 0, EndOffset = 0, Size = 0;
	FILE* f = NULL; // ROM File
	FILE* t = NULL; // Script File output

	if(argc != 6)
	{
		printf("Usage: %s ROM.ext Table.tbl StartOffset(hex) EndOffset(hex) Output.txt\n", argv[0]);
		printf("Example: %s ff1.nes ff1.tbl 28210 2B495 ff1script.txt\n", argv[0]);
		printf("The example will dump all bytes from 28210 to 2B495, including 2B495\n");
		return -1;
	}

	// Open the table
	if(!Dumper.OpenTable(argv[2]))
	{
		printf("Failed to open table %s\n", argv[2]);
		vector<TBL_ERROR> Errors;
		Dumper.GetErrors(Errors);
		for(int i = 0; i < (int)Errors.size(); i++)
		{
			if(Errors[i].LineNumber == -1) // Not on a line means an open error
				printf("#%d: %s\n", i, Errors[i].Description.c_str());
			else
				printf("#%d: Line %d: %s\n", i, Errors[i].LineNumber, Errors[i].Description.c_str());
			return -2;
		}
	}

	// Open the ROM for reading, binary mode
	f = fopen(argv[1], "rb");
	if(f == NULL) // Make sure the ROM file is open
	{
		printf("Failed to open file %s\n", argv[1]);
		return -3;
	}

	// Get the offsets
	StartOffset = strtoul(argv[3], NULL, 16);
	EndOffset = strtoul(argv[4], NULL, 16);
	Size = EndOffset - StartOffset + 1;

	if(StartOffset > EndOffset) // Wrong order
	{
		printf("StartOffset is after EndOffset\n");
		fclose(f);
		return -4;
	}

	// Open the text file
	t = fopen(argv[5], "w");
	if(t == NULL)
	{
		printf("Failed to open script output file %s\n", argv[5]);
		return -3;
	}

	// Get the encoded text from the ROM
	fseek(f, StartOffset, SEEK_SET);

	// Method 1 - Adding text to decode using vectors
	vector<unsigned char> TextBuf; // Expandable array to hold the data
	TextBuf.resize(Size); // Resize the vector to make it the right length (must do if you feed it data through fread)
	fread(&TextBuf[0], 1, Size, f); // Read it into the vector
	Dumper.SetHexBlock(TextBuf);

	// Method 2 - Adding text to decode using a normal array, commented out
	/*
	unsigned char* TextBuf = new unsigned char[Size];
	fread(TextBuf, 1, Size, f);
	Dumper.SetHexBlock(TextBuf, Size);
	delete[] TextBuf;
	*/

	// Print a summary of what's going to be dumped at the top of the text file
	// Includes filename, dump offsets, and time it was dumped
	time_t curtime;
	time(&curtime);
	fprintf(t, "// %s Script Dump [$%X-$%X] - %s\n", argv[1], StartOffset, EndOffset,
		asctime(localtime(&curtime)));

	int StringCount = 0;
	int StringPos = StartOffset; // StringPos is the beginning of the string
	int len = 0; // Length of the string decoded (in hex bytes)
	string TextString = "";   // Decoded string

	while((len = Dumper.DecodeString(TextString, EndString)) != 0) // More to decode
	{
		StringCount++;
		fprintf(t, "// [$%X]\n", StringPos); // Print a comment of where the string was dumped from
		fprintf(t, "%s", TextString.c_str()); // Print the dumped string

		StringPos += len;
	}

	printf("Dumped %d strings\n", StringCount);
	printf("Finished\n");
	return 0;
}

