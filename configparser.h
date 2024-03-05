#pragma once
#include "pch.h"

#include "airport.h"
#include "nlohmann/json.hpp"

#include <string>
#include <map>

using json = nlohmann::ordered_json;
/**
 * @brief Class to manage a single flightplan
 *
 */
namespace vsid
{
	class ConfigParser
	{
	public:
		ConfigParser();
		virtual ~ConfigParser();

		/**
		 * @brief Loads the configs for active airports
		 * 
		 * @param activeAirports 
		 * @param savedCustomRules - customRules that are transferred inbetween rwy change updates
		 * @param savedSettings - settings that are transferred inbetween rwy change updates
		 * @param savedAreas - area settings that are transferred inbetween rwy change updates
		 */
		void loadAirportConfig(std::map<std::string, vsid::Airport> &activeAirports,
							std::map<std::string, std::map<std::string, bool>>& savedCustomRules,
							std::map<std::string, std::map<std::string, bool>>& savedSettings,
							std::map<std::string, std::map<std::string, vsid::Area>>& savedAreas
							);
		void loadMainConfig();
		/**
		 * @brief Loads the grp config
		 * 
		 */
		void loadGrpConfig();
		/**
		 * @brief Fetches a specific color from the settings file
		 * 
		 * @param color - name of the key in the settings file
		 * @return COLORREF 
		 */
		COLORREF getColor(std::string color);
		json grpConfig;
	private:
		std::map<std::string, COLORREF> colors;
		json parsedConfig;
		json vSidConfig;
	};
}
