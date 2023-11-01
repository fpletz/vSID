#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
//#include <string>
//#include <vector>
#include <sstream>

#include "EuroScopePlugIn.h"
#include "airport.h"
#include "constants.h"

//testing
//#include "flightplan.h"
//#include "vSID.h"
#include "configparser.h"
#include "utils.h"

namespace vsid
{
	const std::string pluginName = "vSID";
	const std::string pluginVersion = "0.2.0";
	const std::string pluginAuthor = "Philip Maier, O.B.";
	const std::string pluginCopyright = "to be selected";
	const std::string pluginViewAviso = "";

	class ConfigParser;
	struct fplnInfo
	{
		// TEST
		bool set = false;
		std::string atcRwy;
		std::string sid;
		COLORREF sidColor;
		int clmb;
		bool clmbVia;
	};

	/**
	 * @brief Main class communicating with ES
	 *
	 */
	class VSIDPlugin : public EuroScopePlugIn::CPlugIn
	{
	public:
		VSIDPlugin();
		virtual ~VSIDPlugin();

		bool getDebug();
		/**
		 * @brief Extract a sid waypoint. ES GetSidName() is used and the last 2 chars substracted
		 * 
		 * @param FlightPlanData 
		 * @return
		 */
		std::string findSidWpt(EuroScopePlugIn::CFlightPlanData FlightPlanData);
		/**
		 * @brief Search for a matching SID depending on current RWYs in use, SID wpt
		 * and configured SIDs in config
		 * 
		 * @param FlightPlan - Flightplan data from ES
		 * @param atcRwy - The rwy assigned by ATC which shall be considered
		 */
		vsid::sids::sid processSid(EuroScopePlugIn::CFlightPlan FlightPlan, std::string atcRwy = "");
		/**
		 * @brief Tries to set a clean route without SID. SID will then be placed in front
		 * and color codes for the TagItem set. Processed flightplans are stored.
		 * 
		 * @param FlightPlan - Flightplan data from ES
		 * @param checkOnly - If the flightplan should only be validated
		 * @param atcRwy - The rwy assigned by ATC which shall be considered
		 * @param manualSid - manual Sid that has been selected and should be processed
		 */
		void processFlightplan(EuroScopePlugIn::CFlightPlan FlightPlan, bool checkOnly, std::string atcRwy = "", vsid::sids::sid manualSid = {});
		/**
		 * @brief Called with every function call (list interaction) inside ES
		 *
		 * @param FunctionId - registered TagItemFunction Id
		 * @param sItemString
		 * @param Pt
		 * @param Area
		 */
		void OnFunctionCall(int FunctionId, const char* sItemString, POINT Pt, RECT Area);
		void OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize);
		/**
		 * @brief Called when a dot commmand is used and ES couldn't resolve it.
		 * ES then checks this functions to evaluate the command
		 *
		 * @param sCommandLine
		 * @return
		 */
		bool OnCompileCommand(const char* sCommandLine);
		void OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan);
		void OnFlightPlanDisconnect(EuroScopePlugIn::CFlightPlan FlightPlan);
		void OnControllerDisconnect(EuroScopePlugIn::CController Controller);
		/**
		 * @brief Called when the user clicks on the ok button of the runway selection dialog
		 *
		 */
		void OnAirportRunwayActivityChanged();		
		void OnTimer(int Counter);
		
	private:
		//std::vector<vsid::airport> activeAirports;
		std::map<std::string, vsid::airport> activeAirports;
		bool debug;
		std::map<std::string, vsid::fplnInfo> processed;
		vsid::ConfigParser configParser;
		std::string configPath;
		//testing - later to be managed by flightplan annotations (saving sids there / in the route)
		//std::map<std::string, std::string> selectedSIDs;

		/**
		 * @brief Loads and updates the active airports with available configs
		 *
		 */
		void UpdateActiveAirports();
	};
}
