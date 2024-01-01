#pragma once
#include "pch.h"
#include "es/EuroScopePlugIn.h"
#include "sid.h"

#include <string>
#include <vector>

namespace vsid
{
	namespace fpln
	{
		struct info
		{
			bool atcRWY = false;
			bool noFplnUpdate = false;
			bool remarkChecked = false;
			vsid::sids::sid sid = {};
			vsid::sids::sid customSid = {};
		};
		/**
		 * @brief Strip the filed route from SID/RWY and/or SID to have a bare route to populate with set SID.
		 * Any SIDs or RWYs will be deleted and have to be reset.
		 * 
		 * @param filedRoute - filed route as vector with route entries as elements
		 * @param filedSidWpt - the sid (first) waypoint that is filed
		 * @param origin - departure airport icao
		 * @return std::pair<std::string, std::string> - sid by atc (or none) and rwy by atc if present
		 */
		void clean(std::vector<std::string> &filedRoute, const std::string origin, std::string filedSidWpt = "");

		/**
		 * @brief Get only the assigned rwy extracted from the flightplan
		 * 
		 * @param filedRoute 
		 * @param origin 
		 * @return
		 */
		std::pair<std::string, std::string> getAtcBlock(const std::vector<std::string>& filedRoute, const std::string origin);

		/** 
		 * @brief Searches the flightplan remarks for the given string
		 * 
		 * @param fplnData - flightplan data to get the remarks from
		 * @param searchStr - which string to search for
		 * @return true - if the string was found
		 * @return false - if the string was not found
		 */
		bool findRemarks(const EuroScopePlugIn::CFlightPlanData& fplnData, const std::string(& searchStr));

		/**
		 * @brief Removes the given string from the flightplan remarks if present
		 * 
		 * @param fplnData - flightplan data to remove the remarks from
		 * @param searchStr - which string to remove
		 */
		void removeRemark(EuroScopePlugIn::CFlightPlanData& fplnData, const std::string(&toRemove));

		/**
		 * @brief Adds the given string to the flightplan remarks
		 * 
		 * @param fplnData - flightplan data to edit the remarks for
		 * @param searchStr - which string to add
		 */
		void addRemark(EuroScopePlugIn::CFlightPlanData& fplnData, const std::string(& toAdd));
	}
}