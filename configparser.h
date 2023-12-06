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

		//std::vector<std::filesystem::path> loadConfigFiles();
		void loadAirportConfig(std::map<std::string, vsid::airport> &activeAirports,
							std::map<std::string, std::map<std::string, int>> &savedSettings
							);
		void loadMainConfig();
		void loadGrpConfig();
		COLORREF getColor(std::string color);
		json grpConfig;
	private:
		std::map<std::string, COLORREF> colors;
		json parsedConfig;
		json vSidConfig;
	};
}
