#include "pch.h"
#include "utils.h"

std::string CvSIDPluginUtils::ltrim(std::string& string)
{
	string.erase(string.find_last_not_of(' ') + 1);
	return string;
}

std::string CvSIDPluginUtils::rtrim(std::string& string)
{
	string.erase(0, string.find_first_not_of(' '));
	return string;
}

std::string CvSIDPluginUtils::trim(std::string& string)
{
	return CvSIDPluginUtils::ltrim(CvSIDPluginUtils::rtrim(string));
}

std::vector<std::string> CvSIDPluginUtils::splitString(std::string &string) 
{
	std::vector<std::string> elems;
	std::string elem;
	size_t pos = 0;

	string = CvSIDPluginUtils::trim(string);

	while ((pos = string.find(' ')) != std::string::npos)
	{
		elem = string.substr(0, pos);
		elems.push_back(elem);
		string.erase(0, pos + 1);
		CvSIDPluginUtils::ltrim(string);
	}

	elems.push_back(string);

	return elems;
}
