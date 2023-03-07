#pragma once
#include <string>
#include "pch.h"
#include "EuroScopePlugIn.h"

class CvSIDFlightplan
{
public:
	CvSIDFlightplan(EuroScopePlugIn::CFlightPlan* flightplan);
	virtual ~CvSIDFlightplan();

	EuroScopePlugIn::CFlightPlanData flightplanData;
	std::string filedRouteVector;
	std::string filedSid;
	std::string sidWayPoint;

	std::string extractedRoute = "boing";

	void findSidWaypoint();
	std::string getSidWayPoint();
};
