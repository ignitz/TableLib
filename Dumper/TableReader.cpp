#include "TableReader.h"

using namespace std;

const char* HexAlphaNum = "ABCDEFabcdef0123456789";
TBL_ERROR err;

TableReader::TableReader(const TableReaderType type)
{
	DefEndLine = "<LINE>";
	DefEndString = "<END>";
	LongestHex = 1;
	memset(LongestText, 0, 256*4);
	LongestText['<'] = 6; // Length of <LINE>
	ReaderType = type;
	LineNumber = 0;

	if(type == ReadTypeInsert)
		InitHexTable();
}

TableReader::~TableReader()
{
	// Clear the maps
	if(!LookupText.empty())
		LookupText.clear();
	if(!LookupHex.empty())
		LookupHex.clear();
	if(!LinkedEntries.empty())
		LinkedEntries.clear();
}

bool TableReader::OpenTable(const char* TableFilename)
{
	list<string> EntryList;
	string Line;

	ifstream tablefile(TableFilename);

	if(!tablefile.is_open())
	{
		err.LineNumber = -1;
		err.Description = "Table file cannot be opened";
		TableErrors.push_back(err);
		return false;
	}

	// Gather text into the list
	while(!tablefile.eof())
	{
		getline(tablefile, Line);
		EntryList.push_back(Line);
	}

	for(list<string>::iterator i = EntryList.begin(); i != EntryList.end(); i++)
	{
		LineNumber++;
		if(i->length() == 0) // Blank line
			continue;

		switch((*i)[0]) // first character of the line
		{
		case '$':
			parselink(*i);
			break;
		case '(': // Bookmark (not implemented)
			break;
		case '[': // Script dump (not implemented)
			break;
		case '{': // Script insert (not implemented)
			break;
		case '/': // End string value
			parseendstring(*i);
			break;
		case '*': // End line value
			parseendline(*i);
			break;
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'A': case 'B': case 'C': case 'D':
		case 'E': case 'F': // Normal entry value
			parseentry(*i);
			break;
		default:
			err.LineNumber = LineNumber;
			err.Description = "First character of the line is not a recognized table character";
			TableErrors.push_back(err);
			break;
		}
	}

	if(TableErrors.empty())
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
// GetTextValue() - Returns a Text String from a Hexstring from the table
//-----------------------------------------------------------------------------

bool TableReader::GetTextValue(const string& Hexstring, string& RetTextString)
{
	StringPairMapIt it = LookupText.find(Hexstring);
	if(it != LookupText.end()) // Found
	{
		RetTextString = it->second;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// GetHexValue() - Returns a Hex value from a Text string from the table
//-----------------------------------------------------------------------------

bool TableReader::GetHexValue(const string& Textstring, string& RetHexString)
{
	StringPairMapIt it = LookupHex.find(Textstring);
	if(it != LookupHex.end()) // Found
	{
		RetHexString = it->second;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// AddToMaps() - Adds an entry to the table, if there aren't duplicates
//-----------------------------------------------------------------------------

inline bool TableReader::AddToMaps(const string& HexString, const string& TextString)
{
	string ModString = TextString;

	if(ReaderType == ReadTypeDump) // Check for multiple HexString entries (Dumping confliction)
	{
		replacecontrolcodes(ModString); // Will parse for control codes
		StringPairMapIt it = LookupText.lower_bound(HexString);
		if(it != LookupText.end() && !(LookupText.key_comp()(HexString, it->first))) // Multiple entry
		{
			err.LineNumber = LineNumber;
			err.Description = "Hex token part of the string is already in the table.  Multiple entries causes dumping confliction.";
			TableErrors.push_back(err);
			return false;
		}
		else // Dumpers only need to look up text
			LookupText.insert(it, StringPairMap::value_type(HexString, ModString));
	}

	if(ReaderType == ReadTypeInsert) // Check for multiple TextString entries (Insertion confliction)
	{
		removecontrolcodes(ModString); // Will remove control codes
		StringPairMapIt it = LookupHex.lower_bound(ModString);
		if(it != LookupHex.end() && !(LookupHex.key_comp()(ModString, it->first))) // Multiple entry
		{
			err.LineNumber = LineNumber;
			err.Description = "Text token part of the string is already in the table.  Multiple entries causes insertion confliction.";
			TableErrors.push_back(err);
			return false;
		}
		else // Inserters only need to look up hex values
			LookupHex.insert(it, StringPairMap::value_type(ModString, HexString));
	}

	// Update hex/text lengths
	if(LongestHex < (int)HexString.length())
		LongestHex = (int)HexString.length();
	if(ModString.length() > 0)
	{
		if(LongestText[(unsigned char)ModString[0]] < (int)ModString.length())
			LongestText[(unsigned char)ModString[0]] = (int)ModString.length();
	}

	return true;
}

//-----------------------------------------------------------------------------
// InitHexTable() - Adds the <$XX> strings for insertion
//-----------------------------------------------------------------------------

inline void TableReader::InitHexTable()
{
	char textbuf[16];
	char hexbuf[16];

	// Add capital case <$XX> entries to the lookup map
	for(unsigned int i = 0; i < 0x100; i++)
	{
		sprintf(textbuf, "<$%02X>", i);
		sprintf(hexbuf, "%02X", i);
		LookupHex.insert(map<string, string>::value_type(string(textbuf), string(hexbuf)));
	}
	// Add lower case <$xx> entries to the lookup map
	for(unsigned int i = 0x0A; i < 0x100; i += 0x10)
	{
		for(unsigned int j = 0; j < 6; j++)
		{
			sprintf(textbuf, "<$%02x>", i+j);
			sprintf(hexbuf, "%02X", i+j);
			LookupHex.insert(map<string, string>::value_type(string(textbuf), string(hexbuf)));
		}
	}
}

//-----------------------------------------------------------------------------
// parseendline() - parses a break line table value: ex, *FE
// You can also define messages like *FE=<End Text>
//-----------------------------------------------------------------------------

inline bool TableReader::parseendline(string line)
{
	line.erase(0, 1);
	size_t pos = line.find_first_not_of(HexAlphaNum, 0);
	string hexstr;
	string textstr;
	
	if(pos == string::npos) // Non-alphanum characters not found, *FE type entry?
	{
		textstr= DefEndLine;
		hexstr = line;
		return false;
	}
	else
		hexstr = line.substr(0, pos);

	if((hexstr.length() % 2) != 0)
	{
		err.LineNumber = LineNumber;
		err.Description = "Hex token length is not a multiple of 2";
		TableErrors.push_back(err);
		return false;
	}

	pos = line.find_first_of("=", 0);
	if(pos == string::npos) // No equal sign means it's a default entry
		textstr = DefEndLine;
	else
	{
		line.erase(0, pos+1);
		textstr = line;
	}

	return AddToMaps(hexstr, textstr);
}

//-----------------------------------------------------------------------------
// parseendstring() - parses a string break table value
// Ones like /FF=<end>, /FF, and /<end> (blank string value)
//-----------------------------------------------------------------------------

inline bool TableReader::parseendstring(string line)
{
	line.erase(0, 1);
	size_t pos = line.find_first_of(HexAlphaNum, 0);

	string hexstr;
	string textstr;

	if(pos != 0) // /<end> type entry
	{
		EndTokens.push_back(line);
		return AddToMaps("", line);
	}

	pos = line.find_first_not_of(HexAlphaNum, 0);
	hexstr = line.substr(0, pos);

	if((hexstr.length() % 2) != 0)
	{
		err.LineNumber = LineNumber;
		err.Description = "Hex token length is not a multiple of 2";
		TableErrors.push_back(err);
		return false;
	}

	pos = line.find_first_of("=", 0);
	if(pos == string::npos) // No "=" found, a /FF type entry
		textstr = DefEndString;
	else
	{
		line.erase(0, pos+1);
		textstr = line;
		if(textstr.length() == 0)
		{
			err.LineNumber = LineNumber;
			err.Description = "Blank right side strings for /FF=<end> aren't allowed";
			TableErrors.push_back(err);
			return false;
		}
	}

	string ModString = line;
	removecontrolcodes(ModString);
	EndTokens.push_back(ModString); // Only the inserter uses this

	return AddToMaps(hexstr, textstr);
}

//-----------------------------------------------------------------------------
// parseentry() - parses a hex=text line
//-----------------------------------------------------------------------------

inline bool TableReader::parseentry(string line)
{
	size_t pos = line.find_first_not_of(HexAlphaNum, 0);

	if(pos == string::npos)
	{
		err.LineNumber = LineNumber;
		err.Description = "Entry only contains hex values";
		TableErrors.push_back(err);
		return false;
	}

	string hexstr = line.substr(0, pos);

	if((hexstr.length() % 2) != 0)
	{
		err.LineNumber = LineNumber;
		err.Description = "Hex token length is not a multiple of 2";
		TableErrors.push_back(err);
		return false;
	}

	string textstr;
	pos = line.find_first_of("=", 0);
	if(pos == line.length() - 1) // End of the line, blank entry means it's an error
	{
		err.LineNumber = LineNumber;
		err.Description = "Contains a blank entry";
		TableErrors.push_back(err);
		return false;
	}
	else
	{
		line.erase(0, pos+1);
		textstr = line;
	}

	return AddToMaps(hexstr, textstr);
}

//-----------------------------------------------------------------------------
// parselink() - Parsed a linked entry, like $10F0=<text>,2
//             - Prints both the <text> part and 2 bytes afterwards
//-----------------------------------------------------------------------------

inline bool TableReader::parselink(string line)
{
	LINKED_ENTRY l;
	size_t pos;

	line.erase(0, 1);
	pos = line.find_first_not_of(HexAlphaNum, 0);

	string hexstr = line.substr(0, pos);
	if((hexstr.length() % 2) != 0)
	{
		err.LineNumber = LineNumber;
		err.Description = "Hex token length is not a multiple of 2";
		TableErrors.push_back(err);
		return false;
	}

	string textstr;
	pos = line.find_first_of("=", 0);

	if(pos == string::npos)
	{
		err.LineNumber = LineNumber;
		err.Description = "No equal sign, linked entry format is $XX=<text>,num";
		TableErrors.push_back(err);
		return false;
	}

	line.erase(0, pos+1);

	pos = line.find_first_of(",",0);

	if(pos == string::npos)
	{
		err.LineNumber = LineNumber;
		err.Description = "No comma, linked entry format is $XX=<text>,num";
		TableErrors.push_back(err);
		return false;
	}

	textstr = line.substr(0, pos);
	line.erase(0, pos+1);

	pos = line.find_first_not_of("0123456789", 0);
	if(pos != string::npos)
	{
		err.LineNumber = LineNumber;
		err.Description = "Nonnumeric characters in num field, linked entry format is $XX=<text>,num";
		TableErrors.push_back(err);
		return false;
	}

	l.Text = textstr;
	l.Number = strtoul(line.c_str(), NULL, 10);

	if(ReaderType == ReadTypeDump) // Add linked entry to the dumper
	{
		replacecontrolcodes(l.Text);
		LinkedEntryMapIt it = LinkedEntries.lower_bound(hexstr);
		if(it != LinkedEntries.end() && !(LinkedEntries.key_comp()(hexstr, it->first))) // Multiple entries
		{
			err.LineNumber = LineNumber;
			err.Description = "Hex token part of the linked entry is already in the table.  Multiple entries causes dumping confliction.";
			TableErrors.push_back(err);
			return false;
		}
		else // Inserters only need to look up hex values
			LinkedEntries.insert(it, LinkedEntryMap::value_type(hexstr, l));
	}
	else if(ReaderType == ReadTypeInsert) // Add hexstr=textstr portion to the inserter
	{
		string ModString = textstr;
		removecontrolcodes(ModString); // Will remove control codes
		StringPairMapIt it = LookupHex.lower_bound(ModString);
		if(it != LookupHex.end() && !(LookupHex.key_comp()(ModString, it->first))) // Check for multiple TextString entries (Insertion confliction)
		{
			err.LineNumber = LineNumber;
			err.Description = "Text token part of the string is already in the table.  Multiple entries causes insertion confliction.";
			TableErrors.push_back(err);
			return false;
		}
		else // Inserters only need to look up hex values
		{
			LookupHex.insert(it, StringPairMap::value_type(ModString, hexstr));
			if(LongestHex < (int)hexstr.length())
				LongestHex = (int)hexstr.length();
			if(ModString.length() > 0)
			{
				if(LongestText[(unsigned char)ModString[0]] < (int)ModString.length())
					LongestText[(unsigned char)ModString[0]] = (int)ModString.length();
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// replacecontrolcodes() - Changes \n text to a '\n' character, dumping purpose
// Note - Linked entries use this inside of the parse code, the other entry
//        types use it in AddToMaps
//-----------------------------------------------------------------------------

void TableReader::replacecontrolcodes(string& text)
{
	size_t pos = text.find("\\n", 0);

	while(pos != string::npos) // Replace every occurance
	{
		text.erase(pos, 1); // Erase the \ part
		text[pos] = '\n'; // Replace the leftover n with '\n'
		pos = text.find("\\n", pos); // Search for the next
	}
}

//-----------------------------------------------------------------------------
// removecontrolcodes() - Deletes \n control codes because they require too
//                        strict formatting
//-----------------------------------------------------------------------------

void TableReader::removecontrolcodes(string& text)
{
	size_t pos = text.find("\\n", 0);

	while(pos != string::npos) // Replace every occurance
	{
		text.erase(pos, 2); // Erase the \n
		pos = text.find("\\n", pos); // Search for the next
	}
}