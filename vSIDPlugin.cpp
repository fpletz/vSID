#include "pch.h"
#include "vSIDPlugin.h"
#include "flightplan.h"
#include "utils.h"

CvSIDPlugin::CvSIDPlugin() : EuroScopePlugIn::CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, pluginName.c_str(), pluginVersion.c_str(), pluginAuthor.c_str(), pluginCopyright.c_str()) {

	RegisterTagItemType("Check SID", TAG_ITEM_VSID_CHECK);
	RegisterTagItemFunction("Check SID", TAG_FUNC_VSID_CHECK);

	DisplayUserMessage(pluginName.c_str(), "", std::string("Version " + pluginVersion + " loaded").c_str(), true, true, false, false, false);
}

CvSIDPlugin::~CvSIDPlugin() {}

void CvSIDPlugin::OnFunctionCall(int FunctionId, const char * sItemString, POINT Pt, RECT Area) {
	EuroScopePlugIn::CFlightPlan flightPlanASEL = FlightPlanSelectASEL();

	CvSIDFlightplan* flightplan = new CvSIDFlightplan(&flightPlanASEL);
	//EuroScopePlugIn::CFlightPlanData flightplanData = flightPlanASEL.GetFlightPlanData();

	if (FunctionId == TAG_FUNC_VSID_CHECK) {
		// WORKING EXAMPLE BELOW
		//EuroScopePlugIn::CFlightPlanData flightPlanData = flightPlan.GetFlightPlanData();

		//std::string filedRoute = flightPlanData.GetRoute();
		//const char* sidName = flightPlanData.GetSidName();

		/*for (std::string &waypoint : cvsidpluginutils::splitstring(filedroute))
		{
			DisplayUserMessage(pluginname.c_str(), "", waypoint.c_str(), true, true, false, false, false);
		}*/


		//DisplayUserMessage(pluginName.c_str(), "", sidName, true, true, false, false, false);

		//DisplayUserMessage(pluginName.c_str(), "", flightplanData.GetRoute(), true, true, false, false, false);




		flightplan->findSidWaypoint();
		// std::string sidWaypoint = flightplan->getSidWayPoint();
	}

	delete flightplan;
}