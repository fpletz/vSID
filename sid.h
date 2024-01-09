#pragma once

#include <string>
#include <map>

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
			bool climbvia = false;
			int prio = 0;
			bool pilotfiled = false;
			std::string wtc = "";
			std::string engineType = "";
			std::map<std::string, bool> acftType = {};
			int engineCount = 0;
			int mtow = 0;
			std::string customRule = "";
			SIDArea area = {};
			std::string equip = "";
			int lvp = -1;
			int timeFrom = -1;
			int timeTo = -1;
		};
		/**
		 * @brief Gets the SID name (wpt + number + designator)
		 * 
		 * @param sid - the sid object to check
		 */
		std::string getName(const sid& sid);
		/**
		 * @brief Gets the runway associated with a sid
		 * 
		 * @param sid - the sid object
		 * @return runway if single or first runway if multiple
		 */
		std::string getRwy(const sid& sid);
		/**
		 * @brief Checks if a SID object is empty (waypoint is checked)
		 * 
		 * @param sid - the sid object
		 */
		bool isEmpty(const sid& sid);
		/**
		 * @brief Compares if two SIDs are the same
		 * 
		 * @param sid1 - first sid to compare
		 * @param sid2 - second sid to compare
		 * @return true - if waypoint, number and designator match
		 */
		bool operator==(const sid& sid1, const sid& sid2);
		/**
		 * @brief Compares if two SIDs are the different
		 * 
		 * @param sid1 - first sid to compare
		 * @param sid2 - second sid to compare
		 * @return true - if at least one of waypoint, number or designator don't match
		 */
		bool operator!=(const sid& sid1, const sid& sid2);
	}
}
