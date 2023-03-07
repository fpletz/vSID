#include <string>
#include <vector>

#include "pch.h"
#include "EuroScopePlugIn.h"
#include "flightplan.h"
#include "utils.h"

//debugging
#include "vSID.h"
//end debugging

CvSIDFlightplan::CvSIDFlightplan(EuroScopePlugIn::CFlightPlan* flightplan)
{
	EuroScopePlugIn::CFlightPlanData flightplanData = flightplan->GetFlightPlanData();
	std::string extractedRoute = flightplanData.GetRoute();

	std::vector<std::string> filedRouteVector = CvSIDPluginUtils::splitString(std::string(flightplanData.GetRoute()));
	std::string filedSid = flightplanData.GetSidName();
	std::string sidWaypoint;

}

CvSIDFlightplan::~CvSIDFlightplan() {};

void CvSIDFlightplan::findSidWaypoint()
{
	// gpMyPlugin in vSID.cpp - used to send messages from other files
	extern CvSIDPlugin* gpMyPlugin;
	gpMyPlugin->DisplayUserMessage("vSID", "", flightplanData.GetRoute(), true, true, false, false, false);
	gpMyPlugin->DisplayUserMessage("vSID", "", extractedRoute.c_str(), true, true, false, false, false);
	for (std::string& wayPoint : CvSIDPluginUtils::splitString(filedRouteVector))
	{
		if (wayPoint == filedSid)
		{
			//continue;
		}
		if (wayPoint == "GAGSI") // test SID
		{
			sidWayPoint = wayPoint;
			break;
		}
	}
	sidWayPoint = "outside of loop"; // debug purpose only
}

std::string CvSIDFlightplan::getSidWayPoint()
{
	return sidWayPoint;
}

