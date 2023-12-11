#pragma once

#include <string>

namespace vsid
{
	namespace sids
	{
		struct Point
		{
			float lat;
			float lon;
		};

		struct SIDArea
		{
			Point P1;
			Point P2;
			Point P3;
			Point P4;
		};

		struct sid
		{
			std::string waypoint = "";
			char number = ' ';
			std::string designator = "";
			std::string rwy = "";
			int initialClimb = 0;
			int climbvia = 0;
			int prio = 0;
			int pilotfiled = 0;
			std::string wtc = "";
			std::string engineType = "";
			int engineCount = 0;
			int mtow = 0;
			std::string customRule = "";
			SIDArea area = {};
			std::string equip = "";
			int lvp = 0;
			int nightOps = 0;
		};
		std::string getName(const sid& sid);
		bool isEmpty(const sid& sid);
		bool operator==(const sid& sid1, const sid& sid2);
		bool operator!=(const sid& sid1, const sid& sid2);
	}
}
