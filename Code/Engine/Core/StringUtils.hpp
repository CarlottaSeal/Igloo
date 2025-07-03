#pragma once
//-----------------------------------------------------------------------------------------------
#include <string>
#include <vector>

//-----------------------------------------------------------------------------------------------
const std::string Stringf( char const* format, ... );
const std::string Stringf( int maxLength, char const* format, ... );

const std::string LowerAll(std::string const& str);

typedef std::vector< std::string >	Strings;
Strings SplitStringOnDelimiter(std::string const& originalString, char delimiterToSplitOn);

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to);


