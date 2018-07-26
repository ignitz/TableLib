// Inserter - A bare-boned command line inserter programmed using Klarth's TableLib
// Features - Allows comments at the start of a line as-is...about it.  :p
// This is demonstrated as a simple inserter, for something more heavy duty, check out Atlas

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <list>
#include <string>
#include <fstream>
#include "TextEncoder.h"

using namespace std;

int main(int argc, char* argv[])
{
	TextEncoder Inserter;
	FILE* f = NULL; // ROM File
	ifstream t;     // Script File
	int StartOffset = 0; // Offset to begin insertion
	string DialogueBlock; // Holds every string to be encoded by the inserter

	//-------------------------------------------------------------------------
	// Deal with command line arguments
	//

	if(argc != 5)
	{
		printf("Usage: %s ROM.ext Script.txt Table.tbl StartOffset(hex)\n", argv[0]);
		return -1;
	}

	// Open the table
	if(!Inserter.OpenTable(argv[3]))
	{
		printf("Failed to open table %s\n", argv[2]);
		vector<TBL_ERROR> Errors;
		Inserter.GetErrors(Errors);
		for(int i = 0; i < (int)Errors.size(); i++)
		{
			if(Errors[i].LineNumber == -1) // Not on a line means an open error
				printf("#%d: %s\n", i, Errors[i].Description.c_str());
			else
				printf("#%d: Line %d: %s\n", i, Errors[i].LineNumber, Errors[i].Description.c_str());
			return -2;
		}
	}

	f = fopen(argv[1], "rb+"); // Read/write existing file
	if(f == NULL)
	{
		printf("Failed to open file %s\n", argv[1]);
		return -3;
	}

	t.open(argv[2]);
	if(!t.is_open())
	{
		printf("Failed to open script file %s\n", argv[2]);
		return -3;
	}

	StartOffset = (unsigned int)strtoul(argv[4], NULL, 16);

	//
	//-------------------------------------------------------------------------
	
	//-------------------------------------------------------------------------
	// Parse the script to remove comments
	//

	string Line;
	while(!t.eof())
	{
		getline(t, Line);
		if(Line.empty())
			continue;

		// Search for the comment and erase the commented area
		size_t pos = Line.find("//");
		if(pos != string::npos) // Comment on the line
			Line.erase(pos, Line.length() - pos);
		DialogueBlock += Line;
	}

	t.close();

	//
	//-------------------------------------------------------------------------

	//-------------------------------------------------------------------------
	// Encode the text
	//

	unsigned int BadCharOffset = 0;
	int DialogueLength = Inserter.EncodeStream(DialogueBlock, BadCharOffset);
	if(DialogueLength == -1) // Check for a bad character error
	{
		printf("Bad character '%c' (%02X) not found in table\n", DialogueBlock[BadCharOffset], (unsigned int)DialogueBlock[BadCharOffset]);
		return false;
	}

	//
	//-------------------------------------------------------------------------

	//-------------------------------------------------------------------------
	// Write the text to the file
	// Each entry in the StringTable will be a properly separated string
	//

	unsigned int StringPos = StartOffset; // Position of the current string

	for(TblStringListIt i = Inserter.StringTable.begin(); i != Inserter.StringTable.end(); i++)
	{
		fseek(f, StringPos, SEEK_SET);
		fwrite(i->Text.c_str(), i->Text.length(), 1, f);

		// This is where you'd calculate a pointer and write it to the file
		// Since this is a generic dumper utility, I'll keep the example commented out
		// This will be a 16bit NES pointer for ranges $8000 - $FFFF
		// Note: Must add code to handle the pointer table positioning and advancement
		/*
		fseek(f, PointerTablePos, SEEK_SET);
		unsigned short Val = (StringPos - 0x10) & 0xFFFF;
		if(Val < 0x8000)
			Val += 0x8000;
		fwrite(&Val, 2, 1, f);
		PointerTablePos += 2;
		*/

		StringPos += (unsigned int)i->Text.length();
	}

	//
	//-------------------------------------------------------------------------

	int BytesInserted = StringPos - StartOffset;

	printf("Finished - $%X bytes inserted\n", BytesInserted);
	fclose(f);
	return 0;
}