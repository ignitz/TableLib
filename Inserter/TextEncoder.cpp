//-----------------------------------------------------------------------------
// TextEncoder - A text insertion table library by Klarth
// http://rpgd.emulationworld.com/klarth email - stevemonaco@hotmail.com
// Open source and free to use
// Usage: OpenTable first, EncodeStream second, loop on GetStringTable third,
//        then ClearStringTable last
//-----------------------------------------------------------------------------

#include "TextEncoder.h"
#include "TableReader.h"

using namespace std;

// Constructor

TextEncoder::TextEncoder() : Table(ReadTypeInsert)
{
	bAddEndToken = true;
}

TextEncoder::~TextEncoder()
{
}



//-----------------------------------------------------------------------------
// EncodeStream() - Encodes text in a vector to the string tables
//-----------------------------------------------------------------------------

unsigned int TextEncoder::EncodeStream(string& scriptbuf, unsigned int& BadCharOffset)
{
	TBL_STRING TblString;
	string hexstr;
	string subtextstr;
	unsigned char i;
	unsigned int EncodedSize = 0;
	bool bIsEndToken = false;
	map<string,string>::iterator hashit;
	unsigned int BufOffset = 0;
	
	BadCharOffset = 0;
	hexstr.reserve(Table.LongestHex * 2);

	if(scriptbuf.empty())
		return 0;

	if(!StringTable.empty())
	{
		TBL_STRING RestoreStr = StringTable.back();
		if(RestoreStr.EndToken.empty()) // No end string...restore and keep adding
		{
			StringTable.pop_back();
			TblString.Text = RestoreStr.Text;
		}
	}

	while(BufOffset < scriptbuf.size()) // Translate the whole buffer
	{
		bIsEndToken = false;
		i = Table.LongestText[(unsigned char)scriptbuf[BufOffset]]; // Use LUT
		while(i)
		{
			subtextstr = scriptbuf.substr(BufOffset, i);

			bool success = Table.GetHexValue(subtextstr, hexstr);
			if(!success) // if the entry isn't found
			{
				i--;
				continue;
			}

			EncodedSize += (int)hexstr.size() / 2;

			// Search to see if it's an end token, if it is, add to the string table
			for(list<string>::iterator j = Table.EndTokens.begin(); j != Table.EndTokens.end(); j++)
			{
				if(*j == subtextstr)
				{
					bIsEndToken = true;
					if(bAddEndToken)
						AddToTable(hexstr, &TblString);

					TblString.EndToken = subtextstr;
					StringTable.push_back(TblString);
					TblString.EndToken = "";
					TblString.Text.clear();
					break; // Only once
				}
			}

			if(!bIsEndToken)
				AddToTable(hexstr, &TblString);

			BufOffset += i;
			break; // Entry is finished
		}
		if (i == 0) // no entries found
		{
			BadCharOffset = BufOffset;
			return -1;
		}
	}

	// Encode any extra data that doesn't have an EndToken
	if(!TblString.Text.empty())
		StringTable.push_back(TblString);

	return EncodedSize;
}



//-----------------------------------------------------------------------------
// OpenTable() - Opens, Parses, and Loads a file to memory
//-----------------------------------------------------------------------------

bool TextEncoder::OpenTable(const char* TableFilename)
{
	return Table.OpenTable(TableFilename);
}



void TextEncoder::GetErrors(vector<TBL_ERROR>& Errors)
{
	Errors.assign(Table.TableErrors.begin(), Table.TableErrors.end());
}



//-----------------------------------------------------------------------------
// HexToDec(...) - Converts a hex character to its dec equiv
//-----------------------------------------------------------------------------

inline unsigned short HexToDec(char HexChar)
{
	switch(HexChar)
	{
	case '0': return 0;
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	case '6': return 6;
	case '7': return 7;
	case '8': return 8;
	case '9': return 9;
	case 'a': case 'A': return 10;
	case 'b': case 'B': return 11;
	case 'c': case 'C': return 12;
	case 'd': case 'D': return 13;
	case 'e': case 'E': return 14;
	case 'f': case 'F': return 15;
	}

	// Never gets here
	printf("badstr");
	return 15;
}

inline void TextEncoder::AddToTable(string& Hexstring, TBL_STRING* TblStr)
{
	for(unsigned int k = 0; k < Hexstring.length(); k+=2)
		TblStr->Text += (HexToDec(Hexstring[k+1]) | (HexToDec(Hexstring[k]) << 4));
}