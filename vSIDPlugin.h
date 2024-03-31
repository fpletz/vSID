#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <sstream>

#include "es/EuroScopePlugIn.h"
#include "airport.h"
#include "constants.h"

#include "flightplan.h"
#include "configparser.h"
#include "utils.h"

namespace vsid
{
	const std::string pluginName = "vSID";
	const std::string pluginVersion = "0.8.0";
	const std::string pluginAuthor = "Gameagle";
	const std::string pluginCopyright = "(c) 2024";
	const std::string pluginViewAviso = "";

	class ConfigParser;

	/**
	 * @brief Main class communicating with ES
	 *
	 */
	class VSIDPlugin : public EuroScopePlugIn::CPlugIn
	{
	public:
		VSIDPlugin();
		virtual ~VSIDPlugin();

		/**
		 * @brief Extract a sid waypoint. If ES doesn't find a SID the route is compared to available SID waypoints
		 * 
		 * @param FlightPlanData 
		 * @return
		 */
		void detectPlugins();
		std::string findSidWpt(EuroScopePlugIn::CFlightPlanData FlightPlanData);
		/**
		 * @brief Search for a matching SID depending on current RWYs in use, SID wpt
		 * and configured SIDs in config
		 * 
		 * @param FlightPlan - Flightplan data from ES
		 * @param atcRwy - The rwy assigned by ATC which shall be considered
		 */
		vsid::Sid processSid(EuroScopePlugIn::CFlightPlan FlightPlan, std::string atcRwy = "");
		/**
		 * @brief Tries to set a clean route without SID. SID will then be placed in front
		 * and color codes for the TagItem set. Processed flightplans are stored.
		 * 
		 * @param FlightPlan - Flightplan data from ES
		 * @param checkOnly - If the flightplan should only be validated
		 * @param atcRwy - The rwy assigned by ATC which shall be considered
		 * @param manualSid - manual Sid that has been selected and should be processed
		 */
		void processFlightplan(EuroScopePlugIn::CFlightPlan FlightPlan, bool checkOnly, std::string atcRwy = "", vsid::Sid manualSid = {});
		/**
		 * @brief Called with every function call (list interaction) inside ES
		 *
		 * @param FunctionId - registered TagItemFunction Id
		 * @param sItemString
		 * @param Pt
		 * @param Area
		 */
		void OnFunctionCall(int FunctionId, const char* sItemString, POINT Pt, RECT Area);
		/**
		 * @brief Handles events on plane position updates if the flightplan is present in a tagItem
		 * 
		 * @param FlightPlan 
		 * @param RadarTarget 
		 * @param ItemCode 
		 * @param TagData 
		 * @param sItemString 
		 * @param pColorCode 
		 * @param pRGB 
		 * @param pFontSize 
		 */
		void OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize);
		/**
		 * @brief Called when a dot commmand is used and ES couldn't resolve it.
		 * ES then checks this functions to evaluate the command
		 *
		 * @param sCommandLine
		 * @return
		 */
		bool OnCompileCommand(const char* sCommandLine);
		EuroScopePlugIn::CRadarScreen* OnRadarScreenCreated(const char* sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated);
		/**
		 * @brief Called when something is changed in the flightplan (used for route updates)
		 * 
		 * @param FlightPlan 
		 */
		void OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan);
		/**
		 * @brief Called when something is changed in the controller assigned data
		 *
		 * @param FlightPlan - the flight plan reference whose controller assigned data is updated
		 * @param DataType - the type of the data updated (CTR_DATA_TYPE ...)
		 */
		void OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan, int DataType);
		/**
		 * @brief Called when a flightplan disconnects from the network
		 * 
		 * @param FlightPlan 
		 */
		void OnFlightPlanDisconnect(EuroScopePlugIn::CFlightPlan FlightPlan);
		/**
		 * @brief Called when a position for radar target is updated
		 * 
		 * @param RadarTarget
		 */
		void OnRadarTargetPositionUpdate(EuroScopePlugIn::CRadarTarget RadarTarget);
		/**
		 * @brief Called whenever a controller position is updated. ~ every 5 seconds
		 * 
		 * @param Controller 
		 */
		void OnControllerPositionUpdate(EuroScopePlugIn::CController Controller);
		/**
		 * @brief Called if a controller disconnects
		 * 
		 * @param Controller 
		 */
		void OnControllerDisconnect(EuroScopePlugIn::CController Controller);
		/**
		 * @brief Called when the user clicks on the ok button of the runway selection dialog
		 *
		 */
		void OnAirportRunwayActivityChanged();
		/**
		 * @brief Called once a second
		 * 
		 * @param Counter 
		 * @return * void 
		 */
		void OnTimer(int Counter);
		
	private:
		std::map<std::string, vsid::Airport> activeAirports;
		std::map<std::string, vsid::fpln::Info> processed;
		/**
		 * @param std::map<std::string,> callsign
		 * @param std::pair<,bool> fpln is disconnected
		 */
		std::map<std::string, std::pair< std::chrono::utc_clock::time_point, bool>> removeProcessed;
		vsid::ConfigParser configParser;
		std::string configPath;
		std::map<std::string, std::map<std::string, bool>> savedSettings;
		std::map<std::string, std::map<std::string, bool>> savedRules;
		std::map<std::string, std::map<std::string, vsid::Area>> savedAreas;
		// list of ground states set by controllers
		std::string gsList;
		std::map<std::string, std::string> actAtc;
		std::set<std::string> ignoreAtc;
		bool topskyLoaded;
		bool ccamsLoaded;
		EuroScopePlugIn::CRadarScreen* radarScreen; // needed to be able to call ES functions
		/**
		 * @brief Loads and updates the active airports with available configs
		 *
		 */
		void UpdateActiveAirports();
	};
}
