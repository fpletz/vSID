#pragma once

#include "area.h"
#include "sid.h"

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>

namespace vsid
{
	struct Controller
	{
		std::string si = "";
		int facility = 0;
		double freq = 0.0;
	};
	struct Airport
	{
		std::string icao = "";
		int elevation = 0;
		std::vector<std::string> allRwys = {};
		std::set<std::string> depRwys = {};
		std::set<std::string> arrRwys = {};
		std::map<std::string, bool> customRules = {};
		std::map<std::string, vsid::Area> areas = {};
		std::vector<vsid::Sid> sids = {};
		std::vector<vsid::Sid> timeSids = {};
		std::string timezone = "";
		std::map<std::string, int> appSI = {};
		bool arrAsDep = false;
		int transAlt = 0;
		int maxInitialClimb = 0;
		std::map<std::string, bool> settings = {};
		std::map<std::string, vsid::Controller> controllers = {};
		bool forceAuto = false;
		inline bool hasLowerAtc(const EuroScopePlugIn::CController &myself)
		{
			if (std::all_of(controllers.begin(), controllers.end(), [&](auto controller)
				{
					return controller.second.facility > myself.GetFacility();
				}))
			{
				return false;
			}
			else if (myself.GetFacility() >= 5 &&
				std::none_of(controllers.begin(), controllers.end(), [&](auto controller)
					{
						if (controller.second.facility < myself.GetFacility() ||
							(appSI.contains(myself.GetPositionId()) &&
							appSI.contains(controller.second.si) &&
							appSI[myself.GetPositionId()] > appSI[controller.second.si]) ||
							(!appSI.contains(myself.GetPositionId()) &&
							appSI.contains(controller.second.si))
							) return true;
						else return false;
					}))
			{
				return false;
			}
			else return true;
		}
	};
}
