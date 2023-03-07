#pragma once
#include <vector>
#include <string>

namespace CvSIDPluginUtils
{
	std::string ltrim(std::string& string);
	std::string rtrim(std::string& string);
	std::string trim(std::string& string);
	std::vector<std::string> splitString(std::string& string);
}
