#pragma once
#include <stdio.h>
#include <cstring>
#include <string>
#include <vector>
#include <cstdlib>
#include <map>
#include "TableReader.h"

using namespace std;

//-----------------------------------------------------------------------------
// TextDecoder Class
//-----------------------------------------------------------------------------

class TextDecoder
{
public:
	TextDecoder();
	bool OpenTable(const char* TableFilename);
	void SetHexBlock(vector<unsigned char>& hexbuf);
	void SetHexBlock(unsigned char* hexbuf, int Length);
	int DecodeString(string& textstring, const string& endstring);
	void GetErrors(vector<TBL_ERROR>& Errors);

private:
	vector<char> tempbuf; // Holds the hex strings in a textual form, obtained from SetHexBlock
	TableReader Table;
};