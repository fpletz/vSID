/*
vSID is a plugin for the Euroscope controller software on the Vatsim network.
The aim auf vSID is to ease the work of any controller that edits and assigns
SIDs to flightplans.

Copyright (C) 2024 Gameagle (Philip Maier)
Repo @ https://github.com/Gameagle/vSID

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "area.h"
#include "sid.h"

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <chrono>

// dev
#include "messageHandler.h"
// end dev

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
		//bool arrAsDep = false;
		int transAlt = 0;
		int maxInitialClimb = 0;
		std::map<std::string, bool> settings = {};
		std::map<std::string, vsid::Controller> controllers = {};
		std::map<std::string, std::map<std::string, std::pair<int, std::chrono::time_point<std::chrono::utc_clock, std::chrono::minutes>>>> requests = {};
		bool forceAuto = false;

		/**
		 * @brief Checks if another controller with a lower facility is online
		 * 
		 * @param myself - Controller().ControllerMyself()
		 */
		inline bool hasLowerAtc(const EuroScopePlugIn::CController &myself, bool toActivate = false)
		{
			if (std::all_of(controllers.begin(), controllers.end(), [&](auto controller)
				{
					if (toActivate)
					{
						return controller.second.facility > myself.GetFacility();
					}
					else return controller.second.facility >= myself.GetFacility();
					
				}))
			{
				return false;
			}
			else if (myself.GetFacility() >= 5 &&
				std::none_of(controllers.begin(), controllers.end(), [&](auto controller)
					{
						if (controller.second.facility < myself.GetFacility()) return true;
						else if (appSI.contains(myself.GetPositionId()) &&
							appSI.contains(controller.second.si) &&
							(appSI[myself.GetPositionId()] > appSI[controller.second.si] && !toActivate) ||
							appSI[myself.GetPositionId()] >= appSI[controller.second.si] && toActivate)
							 return true;
						else if (!appSI.contains(myself.GetPositionId()) &&
							appSI.contains(controller.second.si)) return true;
						else return false;

						/*if (controller.second.facility < myself.GetFacility() ||
							(appSI.contains(myself.GetPositionId()) &&
								appSI.contains(controller.second.si) &&
								appSI[myself.GetPositionId()] > appSI[controller.second.si]) ||
							(!appSI.contains(myself.GetPositionId()) &&
								appSI.contains(controller.second.si))
							) return true;
						else return false;*/
					}))
			{
				return false;
			}
			else return true;
		}

		/**
		 * @brief Compare operator for custom request sorting.
		 * 
		 * @param second.first - current stored position
		 * @param first - callsign
		 */
		struct compreq
		{
			bool operator()(auto l, auto r) const
			{
				if (l.second.first != r.second.first) return l.second.first < r.second.first;
				return l.first < r.first;
			}
		};

		/**
		 * @brief Sorts requests based on position in queue and updates positions
		 * 
		 */
		inline void sortRequests()
		{
			for (auto& req : this->requests)
			{
				std::set<std::pair<std::string, std::pair<int, std::chrono::time_point<std::chrono::utc_clock, std::chrono::minutes>>>, compreq> setReq(req.second.begin(), req.second.end());

				for (auto it = setReq.begin(); it != setReq.end(); ++it)
				{
					int pos = std::distance(setReq.begin(), it);
					if (req.second.contains(it->first))
					{
						req.second[it->first].first = pos + 1;
					}
				}
			}
		}
	};
}
