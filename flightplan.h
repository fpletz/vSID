#pragma once
#include "pch.h"
#include "EuroScopePlugIn.h"

#include <vector>
#include <string>

namespace vsid
{
	namespace fpln
	{
		/**
		 * @brief Strip the filed route from SID/RWY and/or SID to have a bare route to populate with set SID.
		 * 
		 * @param filedRoute - filed route as vector with route entries as elements
		 * @param filedSidWpt - the sid (first) waypoint that is filed
		 * @param origin - departure airport icao
		 * @return std::pair<std::string, std::string> - sid by atc (or none) and rwy by atc if present
		 */
		std::pair<std::string, std::string> clean(std::vector<std::string> &filedRoute, std::string origin, std::string filedSidWpt = "");
	}
}