#pragma once

#include <string>
//#include <map>
#include <vector>
#include <map>
#include <set>

#include "area.h"
#include "sid.h"

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
		//std::map<std::string, bool> areaSettings = {};
		std::vector<vsid::Sid> sids = {};
		std::vector<vsid::Sid> timeSids = {};
		std::string timezone = "";
		int arrAsDep = 0;
		int transAlt = 0;
		int maxInitialClimb = 0;
		std::map<std::string, bool> settings = {};
		std::map<std::string, vsid::Controller> controllers = {};
	};
}
