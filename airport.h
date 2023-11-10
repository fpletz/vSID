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
		//std::string activeRwys;
		//std::string availableRwys;
		/*std::vector<std::string> actRwys;
		std::vector<std::string> avblRwys;*/
		std::set<std::string> depRwys;
		std::set<std::string> arrRwys;
		std::vector<vsid::sids::sid> sids;
		std::map<std::string, int> customRules;
		int transAlt;
		int maxInitialClimb;
	};
}
