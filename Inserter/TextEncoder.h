#pragma once
#include <fstream>
#include <cstdlib>
#include <utility>
#include <string>
#include <vector>
#include <map>
#include <list>
#include "TableReader.h"

using namespace std;

// Data Structure for a script string
typedef struct TBL_STRING
{
	string Text;
	string EndToken;
} TBL_STRING;

typedef list<TBL_STRING>::iterator TblStringListIt;

//-----------------------------------------------------------------------------
// TextEncoder Interfaces
//-----------------------------------------------------------------------------

class TextEncoder
{
public:
	TextEncoder();
	~TextEncoder();

	bool OpenTable(const char* TableFilename);
	unsigned int EncodeStream(string& scriptbuf, unsigned int& BadCharOffset);
	void GetErrors(vector<TBL_ERROR>& Errors);

	list<TBL_STRING> StringTable; // (Encoded) String table
	vector<string>   EndTokens;   // String end tokens
	bool bAddEndToken; // Determines whether to add the end token to the output

private:
	inline void AddToTable(string& Hexstring, TBL_STRING* TblStr);

	TableReader Table;
};

inline unsigned short HexToDec(char HexChar);