#pragma once

#include <string>
#include <map>

namespace vsid
{
	class Sid
	{
	public:
		/*Sid() : waypoint(""), number(' '), designator(""), rwy(""), initialClimb(0),
				climbvia(false), prio(99), pilotfiled(false), wtc(""), engineType(""),
				acftType(std::map<std::string, bool>{}), engineCount(0), mtow(0),
				customRule(""), equip(""), lvp(-1), timeFrom(-1), timeTo(-1) {};*/

		Sid(std::string waypoint = "", char number = ' ', std::string designator = "",
			std::string rwy = "", int initialClimb = 0, bool climbvia = false, int prio = 99,
			bool pilotfiled = false, std::string wtc = "", std::string engineType = "",
			std::map<std::string, bool>acftType = {}, int engineCount = 0, int mtow = 0,
			std::string customRule = "", std::string area = "", std::string equip = "", int lvp = -1,
			int timeFrom = -1, int timeTo = -1) : waypoint(waypoint), number(number), designator(designator),
			rwy(rwy), initialClimb(initialClimb), climbvia(climbvia), prio(prio),
			pilotfiled(pilotfiled), wtc(wtc), engineType(engineType),
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
		 * @brief Gets the runway associated with a sid
		 *
		 * @param sid - the sid object
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
	//namespace sids
	//{
	//	struct Point
	//	{
	//		float lat;
	//		float lon;
	//	};

	//	struct SIDArea
	//	{
	//		Point P1;
	//		Point P2;
	//		Point P3;
	//		Point P4;
	//	};

	//	struct sid
	//	{
	//		std::string waypoint = "";
	//		char number = ' ';
	//		std::string designator = "";
	//		std::string rwy = "";
	//		int initialClimb = 0;
	//		bool climbvia = false;
	//		int prio = 0;
	//		bool pilotfiled = false;
	//		std::string wtc = "";
	//		std::string engineType = "";
	//		std::map<std::string, bool> acftType = {};
	//		int engineCount = 0;
	//		int mtow = 0;
	//		std::string customRule = "";
	//		SIDArea area = {};
	//		std::string equip = "";
	//		int lvp = -1;
	//		int timeFrom = -1;
	//		int timeTo = -1;
	//	};
	//	/**
	//	 * @brief Gets the SID name (wpt + number + designator)
	//	 * 
	//	 * @param sid - the sid object to check
	//	 */
	//	std::string getName(const sid& sid);
	//	/**
	//	 * @brief Gets the runway associated with a sid
	//	 * 
	//	 * @param sid - the sid object
	//	 * @return runway if single or first runway if multiple
	//	 */
	//	std::string getRwy(const sid& sid);
	//	/**
	//	 * @brief Checks if a SID object is empty (waypoint is checked)
	//	 * 
	//	 * @param sid - the sid object
	//	 */
	//	bool isEmpty(const sid& sid);
	//	/**
	//	 * @brief Compares if two SIDs are the same
	//	 * 
	//	 * @param sid1 - first sid to compare
	//	 * @param sid2 - second sid to compare
	//	 * @return true - if waypoint, number and designator match
	//	 */
	//	bool operator==(const sid& sid1, const sid& sid2);
	//	/**
	//	 * @brief Compares if two SIDs are the different
	//	 * 
	//	 * @param sid1 - first sid to compare
	//	 * @param sid2 - second sid to compare
	//	 * @return true - if at least one of waypoint, number or designator don't match
	//	 */
	//	bool operator!=(const sid& sid1, const sid& sid2);
	//}
}
