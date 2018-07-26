#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <map>

using namespace std;

// Structure for errors
typedef struct TBL_ERROR
{
	int LineNumber; // The line number which the error occurred
	string Description; // A description of what the error was
} TBL_ERROR;

// Data structure for a linked entry
typedef struct LINKED_ENTRY
{
	string Text;
	int Number;
} LINKED_ENTRY;

typedef map<string,string> StringPairMap;
typedef StringPairMap::iterator StringPairMapIt;
typedef map<string,LINKED_ENTRY> LinkedEntryMap;
typedef map<string,LINKED_ENTRY>::iterator LinkedEntryMapIt;

enum TableReaderType { ReadTypeInsert, ReadTypeDump };

class TableReader
{
public:
	TableReader(const TableReaderType type);
	~TableReader();

	bool OpenTable(const char* TableFilename);

	bool GetTextValue(const string& Hexstring, string& RetTextString);
	bool GetHexValue(const string& Textstring, string& RetHexString);

	vector<TBL_ERROR> TableErrors;

	int LongestHex;       // The longest hex entry, in bytes
	int LongestText[256]; // LUT for the longest text string starting with the first character
	LinkedEntryMap LinkedEntries;  // $FF=<text>,num type entries (Dumping)
	list<string> EndTokens;        // End string tokens

private:
	inline bool parseendline(string line);
	inline bool parseendstring(string line);
	inline bool parseentry(string line);
	inline bool parselink(string line);
	void replacecontrolcodes(string& text);
	void removecontrolcodes(string& text);

	inline bool AddToMaps(const string& HexString, const string& TextString);
	inline void InitHexTable();

	string DefEndLine;
	string DefEndString;

	StringPairMap LookupText; // for looking up text values.  (Dumping)
	StringPairMap LookupHex;  // for looking up hex values.  (Insertion)

	int LineNumber;

	TableReaderType ReaderType;
};