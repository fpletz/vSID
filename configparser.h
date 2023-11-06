#pragma once
#include "pch.h"

#include <string>
#include <vector>
#include <fstream>

//#include "vSID.h"
#include "airport.h"
#include "nlohmann/json.hpp"

//#include <filesystem>

//#include <fstream>

//#include "messageHandler.h"
//#include "utils.h"


//#include "EuroScopePlugIn.h"
//#include "utils.h"

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
		void loadAirportConfig(std::map<std::string, vsid::airport> &activeAirports);
		void loadMainConfig();
		void loadGrpConfig();
		COLORREF getColor(std::string color);
	private:
		std::map<std::string, COLORREF> colors;
		json parsedConfig;
		json vSidConfig;
		json grpConfig;
	};
}
