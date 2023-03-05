#include "pch.h"
#include "vSIDPlugin.h"

CvSIDPlugin::CvSIDPlugin() : EuroScopePlugIn::CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, pluginName.c_str(), pluginVersion.c_str(), pluginAuthor.c_str(), pluginCopyright.c_str()) {

	RegisterTagItemType("Check SID", TAG_ITEM_VSID_CHECK);
	RegisterTagItemFunction("Check SID", TAG_FUNC_VSID_CHECK);

	DisplayUserMessage(pluginName.c_str(), "", std::string("Version " + pluginVersion + " loaded").c_str(), true, true, false, false, false);
}

CvSIDPlugin::~CvSIDPlugin() {}

void CvSIDPlugin::OnFunctionCall(int FunctionId, const char * sItemString, POINT Pt, RECT Area) {
	EuroScopePlugIn::CFlightPlan flightPlan = FlightPlanSelectASEL();

	if (FunctionId == TAG_FUNC_VSID_CHECK) {
		EuroScopePlugIn::CFlightPlanData flightPlanData = flightPlan.GetFlightPlanData();

		std::string filedRoute = flightPlanData.GetRoute();
		const char* sidName = flightPlanData.GetSidName();

		DisplayUserMessage(pluginName.c_str(), "", filedRoute.c_str(), true, true, false, false, false);
		DisplayUserMessage(pluginName.c_str(), "", sidName, true, true, false, false, false);
	}
}
