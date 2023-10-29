#include "pch.h"
//#include "EuroScopePlugIn.h"
#include "flightplan.h"

#include <string>
//#include <vector>
//#include <array>






//
////bool CvSIDFlightplan::validatePosition(struct sid* sid)
////{
////	//EuroScopePlugIn::CRadarTargetPositionData trackPosition = this->flightPlan.GetFPTrackPosition();
////	
////	Point trackPosition = { 34.67, 16.74 }; //randome Point for debugging until I find out how to convert trackposition to lat/lon coordinates
////
////
////	line edge1 = { sid->area.P1, sid->area.P2 };
////	line edge2 = { sid->area.P2, sid->area.P3 };
////	line edge3 = { sid->area.P3, sid->area.P4 };
////	line edge4 = { sid->area.P4, sid->area.P1 };
////
////	std::array<line, 4> edges = { edge1, edge2, edge3, edge4 };
////
////	line ray = { trackPosition , {trackPosition.lon, trackPosition.lat + 0.1} }; //0.1 deg lat is approx 10 km which should be enaugh for an infinte ray in an Airport Area
////	
////	for (line edge : edges) {
////
////		int mOfEdge = (edge.P2.lon - edge.P1.lon) / (edge.P2.lat - edge.P1.lat);
////
////
////	}
////
////
////	return true;
////}

