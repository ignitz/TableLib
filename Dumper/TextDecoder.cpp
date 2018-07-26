//---------------------------------------------------------------------------------------
// TextDecoder - A table library by Klarth, http://rpgd.emulationworld.com/klarth
// email - stevemonaco@hotmail.com
// Open source and free to use / modify
// Usage: OpenTable first, SetHexBlock second, loop with DecodeString third
//---------------------------------------------------------------------------------------


#include "TextDecoder.h"
#include "TableReader.h"

using namespace std;

TextDecoder::TextDecoder() : Table(ReadTypeDump)
{
}

//-----------------------------------------------------------------------------
// DecodeString() - Decodes a text string from a vector, then removes the
//                  hex data from the data supplied by SetHexBlock
//                  Returns - Number of hex characters decoded
//                          - -1 when a linked entry goes past the end of input
//-----------------------------------------------------------------------------

int TextDecoder::DecodeString(string& textstring, const string& endstring)
{
	unsigned int hexoff = 0, hexsize = 0;
	string hexstr, textstr;
	unsigned int i;

	textstring = ""; // Clear the textstring

	// Get the size of the buffer
	hexsize = (unsigned int)tempbuf.size();

	if(hexsize == 0)
		return 0;

	// Convert the text hex stream into a text stream and store it in textbuf
	while(hexoff < hexsize)
	{
		i = Table.LongestHex * 2;
		if(i > (hexsize - hexoff))
			i = hexsize - hexoff;

		while(i > 0)
		{
			hexstr = "";
			textstr = "";
			int sizeleft = (int)tempbuf.size() - (int)hexoff;
			for(unsigned int j = 0; (j < i) && (tempbuf.size() > hexoff+j); j++)
				hexstr += tempbuf[hexoff+j];

			// Lookup the hex string in the map
			bool success = Table.GetTextValue(hexstr, textstr);
			if(!success) // not in normal entries
			{
				// Try to search a linked entry
				LinkedEntryMapIt linkedit = Table.LinkedEntries.find(hexstr);
				if(linkedit != Table.LinkedEntries.end()) // Found a linked entry
				{
					int hexbytes = (int)linkedit->first.length() + linkedit->second.Number * 2;
					if(hexbytes <= sizeleft)
					{
						// Write the text part of the linked entry
						for(size_t j = 0; j < linkedit->second.Text.length(); j++)
							textstring += linkedit->second.Text[j];

						hexoff += (int)linkedit->first.length();

						// Write the bytes following
						for(int j = 0; j < linkedit->second.Number; j++)
						{
							textstring += "<$";
							textstring += tempbuf[hexoff+j*2];
							textstring += tempbuf[hexoff+j*2+1];
							textstring += ">";
						}
						hexoff += linkedit->second.Number * 2;
					}
					else
						return -1;
					break;					
				}
				else
				{
					i-=2;
					continue;
				}
			}
			
			// Found in the table, put it in the text buffer
			for(unsigned int j = 0; j < textstr.length(); j++)
				textstring += textstr[j];

			// Increase the hex offset by how long the hex string was
			hexoff += (unsigned int)hexstr.length();

			if(textstr == endstring)
			{
				vector<char>::iterator v = tempbuf.begin() + hexoff;
				tempbuf.erase(tempbuf.begin(), v);
				return (hexoff/2);
			}

			break;
		}

		if(i == 0) // no matching entry, print out a <$00> entry
		{
			textstring += "<$";
			textstring += tempbuf[hexoff];
			textstring += tempbuf[hexoff+1];
			textstring += ">";
			hexoff += 2;
		}
	}

	vector<char>::iterator v = tempbuf.begin() + hexoff;
	tempbuf.erase(tempbuf.begin(), v);
	return (hexoff / 2);
}

//-----------------------------------------------------------------------------
// SetHexBlock() - Sets the hex block for DecodeString usage
//-----------------------------------------------------------------------------

void TextDecoder::SetHexBlock(vector<unsigned char>& hexbuf)
{
	char conbuf[4];
	memset(conbuf, 0, 4);

	vector<char> resizebuf; // Used for resizing the internal buffer of tempbuf
	resizebuf.reserve(hexbuf.size() * 2);
	tempbuf.clear();
	tempbuf.swap(resizebuf);

	// Change a hex stream to a "text hex" stream
	// ie. 0x35A0 to "35A0"
	for(unsigned int i = 0; i < (unsigned int)hexbuf.size(); i++)
	{
		sprintf(conbuf, "%02X", (unsigned char)hexbuf[i]);
		tempbuf.push_back(conbuf[0]); tempbuf.push_back(conbuf[1]);
	}
}

//-----------------------------------------------------------------------------
// SetHexBlock() - Sets the hex block for DecodeString usage
//-----------------------------------------------------------------------------

void TextDecoder::SetHexBlock(unsigned char* hexbuf, int Length)
{
	char conbuf[4];
	memset(conbuf, 0, 4);

	vector<char> resizebuf; // Used for resizing the internal buffer of tempbuf, else it could bloat
	resizebuf.reserve(Length * 2);
	tempbuf.clear();
	tempbuf.swap(resizebuf);

	// Change a hex stream to a "text hex" stream
	// ie. 0x35A0 to "35A0"
	for(unsigned char* i = hexbuf; i < hexbuf+Length; i++)
	{
		sprintf(conbuf, "%02X", *i);
		tempbuf.push_back(conbuf[0]); tempbuf.push_back(conbuf[1]);
	}
}

//-----------------------------------------------------------------------------
// OpenTable() - Opens, Parses, and Loads a file to memory
//-----------------------------------------------------------------------------

bool TextDecoder::OpenTable(const char* TableFilename)
{
	return Table.OpenTable(TableFilename);
}

void TextDecoder::GetErrors(vector<TBL_ERROR>& Errors)
{
	Errors.assign(Table.TableErrors.begin(), Table.TableErrors.end());
}