#pragma once

#include <string>
#include <map>

namespace vsid
{
	class Sid
	{
	public:

		Sid(std::string waypoint = "", char number = ' ', std::string designator = "",
			std::string rwy = "", int initialClimb = 0, bool climbvia = false, int prio = 99,
			bool pilotfiled = false, std::map<std::string, std::string> actArrRwy = {}, std::map<std::string, std::string> actDepRwy = {}, std::string wtc = "", std::string engineType = "",
			std::map<std::string, bool>acftType = {}, int engineCount = 0, int mtow = 0,
			std::string customRule = "", std::string area = "", std::string equip = "", int lvp = -1,
			int timeFrom = -1, int timeTo = -1) : waypoint(waypoint), number(number), designator(designator),
			rwy(rwy), initialClimb(initialClimb), climbvia(climbvia), prio(prio),
			pilotfiled(pilotfiled), actArrRwy(actArrRwy), actDepRwy(actDepRwy), wtc(wtc), engineType(engineType),
			acftType(acftType), engineCount(engineCount), mtow(mtow),
			customRule(customRule), area(area), equip(equip), lvp(lvp), timeFrom(timeFrom), timeTo(timeTo) {};

		std::string waypoint;
		char number;
		std::string designator;
		std::string rwy;
		int initialClimb;
		bool climbvia;
		int prio;
		bool pilotfiled;
		std::map<std::string, std::string> actArrRwy;
		std::map<std::string, std::string> actDepRwy;
		std::string wtc;
		std::string engineType;
		std::map<std::string, bool> acftType;
		int engineCount;
		int mtow;
		std::string customRule;
		std::string area;
		std::string equip;
		int lvp;
		int timeFrom;
		int timeTo;
		/**
		 * @brief Gets the SID name (wpt + number + designator)
		 *
		 * @param sid - the sid object to check
		 */
		std::string name() const;
		/**
		 * @brief Return the full name of a sid (including indexed designators)
		 * 
		 */
		std::string fullName() const;
		/**
		 * @brief Gets the runway associated with a sid
		 *
		 * @return runway if single or first runway if multiple
		 */
		std::string getRwy() const;
		/**
		 * @brief Checks if a SID object is empty (waypoint is checked)
		 *
		 * @param sid - the sid object
		 */
		bool empty() const;
		/**
		 * @brief Compares if two SIDs are the same
		 *
		 * @param sid1 - first sid to compare
		 * @param sid2 - second sid to compare
		 * @return true - if waypoint, number and designator match
		 */
		bool operator==(const Sid& sid);
		/**
		 * @brief Compares if two SIDs are the different
		 *
		 * @param sid1 - first sid to compare
		 * @param sid2 - second sid to compare
		 * @return true - if at least one of waypoint, number or designator don't match
		 */
		bool operator!=(const Sid& sid);
	};
}
