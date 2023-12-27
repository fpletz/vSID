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
		std::string icao = "";
		int elevation = 0;
		std::vector<std::string> allRwys = {};
		std::set<std::string> depRwys = {};
		std::set<std::string> arrRwys = {};
		std::map<std::string, int> customRules = {};
		std::vector<vsid::sids::sid> sids = {};
		std::vector<vsid::sids::sid> nightSids = {};
		std::string timezone = "";
		int arrAsDep = 0;
		int transAlt = 0;
		int maxInitialClimb = 0;
		std::map<std::string, int> settings = {};
	};
}
