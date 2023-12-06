#pragma once

#include <string>
//#include <map>
#include <vector>
#include <map>
#include <set>

#include "sid.h"

namespace vsid
{
	struct airport
	{
		std::string icao;
		int elevation;
		std::vector<std::string> allRwys;
		std::set<std::string> depRwys;
		std::set<std::string> arrRwys;
		std::vector<vsid::sids::sid> sids;
		std::map<std::string, int> customRules;
		int arrAsDep;
		int transAlt;
		int maxInitialClimb;
		std::map<std::string, int> settings;
	};
}
