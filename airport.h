#pragma once

#include <string>
//#include <map>
#include <vector>

#include "sid.h"

namespace vsid
{
	struct airport
	{
		std::string icao;
		int elevation;
		//std::string activeRwys;
		//std::string availableRwys;
		std::vector<std::string> actRwys;
		std::vector<std::string> avblRwys;
		std::vector<vsid::sids::sid> sids;
		int transAlt;
		int maxInitialClimb;
	};
}
