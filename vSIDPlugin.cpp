#include "pch.h"

#include "vSIDPlugin.h"
#include "flightplan.h"
#include "timeHandler.h"
#include "messageHandler.h"
#include "area.h"

#include <set>
#include <algorithm>

vsid::VSIDPlugin* vsidPlugin;

vsid::VSIDPlugin::VSIDPlugin() : EuroScopePlugIn::CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, pluginName.c_str(), pluginVersion.c_str(), pluginAuthor.c_str(), pluginCopyright.c_str()) {

	this->configParser.loadMainConfig();
	this->configParser.loadGrpConfig();
	this->gsList = "STUP,PUSH,TAXI,DEPA";
	
	RegisterTagItemType("vSID SID", TAG_ITEM_VSID_SIDS);
	RegisterTagItemFunction("SIDs Auto Select", TAG_FUNC_VSID_SIDS_AUTO);
	RegisterTagItemFunction("SIDs Menu", TAG_FUNC_VSID_SIDS_MAN);

	RegisterTagItemType("vSID CLMB", TAG_ITEM_VSID_CLIMB);
	RegisterTagItemFunction("Climb Menu", TAG_FUNC_VSID_CLMBMENU);

	RegisterTagItemType("vSID RWY", TAG_ITEM_VSID_RWY);
	RegisterTagItemFunction("RWY Menu", TAG_FUNC_VSID_RWYMENU);

	UpdateActiveAirports(); // preload rwy settings

	DisplayUserMessage("Message", "vSID", std::string("Version " + pluginVersion + " loaded").c_str(), true, true, false, false, false);
}

vsid::VSIDPlugin::~VSIDPlugin() {}

/*
* BEGIN OWN FUNCTIONS
*/

bool vsid::VSIDPlugin::getDebug() const
{
	return this->debug;
}

std::string vsid::VSIDPlugin::findSidWpt(EuroScopePlugIn::CFlightPlanData FlightPlanData)
{
	std::string filedSid = FlightPlanData.GetSidName();
	std::vector<std::string> filedRoute = vsid::utils::split(FlightPlanData.GetRoute(), ' ');
	std::string sidWpt = "";

	if (filedRoute.size() == 0) return "";
	if (filedSid != "")
	{
		return filedSid.substr(0, filedSid.length() - 2);
	}
	else
	{
		std::set<std::string> sidWpts = {};
		for (const vsid::Sid &sid : this->activeAirports[FlightPlanData.GetOrigin()].sids)
		{
			sidWpts.insert(sid.waypoint);
		}
		for (std::string &wpt : filedRoute)
		{
			if (wpt.find("/") != std::string::npos)
			{
				wpt = vsid::utils::split(wpt, '/').front();
			}
			if (std::any_of(sidWpts.begin(), sidWpts.end(), [&](auto item)
				{
					return item == wpt;
				}))
			{
				return wpt;
			}
		}
	}
	return "";
}

vsid::Sid vsid::VSIDPlugin::processSid(EuroScopePlugIn::CFlightPlan FlightPlan, std::string atcRwy)
{
	EuroScopePlugIn::CFlightPlan fpln = FlightPlan;
	EuroScopePlugIn::CFlightPlanData fplnData = fpln.GetFlightPlanData();
	EuroScopePlugIn::CFlightPlanControllerAssignedData cad = fpln.GetControllerAssignedData();
	std::vector<std::string> filedRoute = vsid::utils::split(std::string(fplnData.GetRoute()), ' ');
	std::string sidWpt = vsid::VSIDPlugin::findSidWpt(fplnData);
	vsid::Sid setSid = {};
	int prio = 99;
	bool customRuleActive = false;
	std::set<std::string> wptRules = {};
	std::set<std::string> actTSid = {};

	if (this->activeAirports[fplnData.GetOrigin()].customRules.size() > 0)
	{
		customRuleActive = std::any_of(
			this->activeAirports[fplnData.GetOrigin()].customRules.begin(),
			this->activeAirports[fplnData.GetOrigin()].customRules.end(),
			[](auto item)
			{
				return item.second;
			}
		);
	}

	if (customRuleActive)
	{
		std::vector<std::string> sidRules = {};
		std::map<std::string, bool> customRules = this->activeAirports[fplnData.GetOrigin()].customRules;
		for (auto it = this->activeAirports[fplnData.GetOrigin()].sids.begin(); it != this->activeAirports[fplnData.GetOrigin()].sids.end();)
		{
			if (it->customRule == "" || wptRules.contains(it->waypoint)) ++it;
			if (it == this->activeAirports[fplnData.GetOrigin()].sids.end()) break;

			if (it->customRule.find(',') != std::string::npos)
			{
				sidRules = vsid::utils::split(it->customRule, ',');
			}
			else sidRules = { it->customRule };
			
			if (std::any_of(customRules.begin(), customRules.end(), [&](auto item)
				{
					return std::find(sidRules.begin(), sidRules.end(), item.first) != sidRules.end() && item.second;
				}
			))
			{
				wptRules.insert(it->waypoint);
			}
			++it;
		}
	}

	if (this->activeAirports[fplnData.GetOrigin()].settings["time"] &&
		this->activeAirports[fplnData.GetOrigin()].timeSids.size() > 0 &&
		std::any_of(this->activeAirports[fplnData.GetOrigin()].timeSids.begin(),
			this->activeAirports[fplnData.GetOrigin()].timeSids.end(),
			[&](auto item)
			{
				return sidWpt == item.waypoint;
			}
			)
		)
	{
		for (const vsid::Sid& sid : this->activeAirports[fplnData.GetOrigin()].timeSids)
		{
			if (sid.waypoint != sidWpt) continue;
			if (vsid::time::isActive(this->activeAirports[fplnData.GetOrigin()].timezone, sid.timeFrom, sid.timeTo))
			{
				actTSid.insert(sid.waypoint);
			}
		}
	}

	for(vsid::Sid &currSid : this->activeAirports[fplnData.GetOrigin()].sids)
	{
		bool rwyMatch = false;
		bool restriction = false;
		// skip if current SID does not match found SID wpt
		if (currSid.waypoint != sidWpt)
		{	
			continue;
		}
		// skip if current SID rwys don't match dep rwys
		for (std::string depRwy : this->activeAirports[fplnData.GetOrigin()].depRwys)
		{
			// skip if a rwy has been set manually and it doesn't match available sid rwys
			if (atcRwy != "" && atcRwy != depRwy)
			{
				continue;
			}
			// skip if airport dep rwys are not part of the SID
			if (currSid.rwy.find(depRwy) == std::string::npos)
			{
				continue;
			}
			else
			{
				rwyMatch = true;
				break;
			}
		}

		// skip if custom rules are active but the current sid has no rule or has a rule but this is not active
		if (customRuleActive &&
			(wptRules.contains(currSid.waypoint) && currSid.customRule == "") ||
			(!wptRules.contains(currSid.waypoint) && currSid.customRule != ""))
		{
			continue;
		}
		// skip if custom rules are inactive but a rule exists in sid
		if (!customRuleActive && currSid.customRule != "")
		{
			continue;
		}

		if (currSid.area != "")
		{
			std::string icao = fplnData.GetOrigin();
			std::vector<std::string> sidAreas = vsid::utils::split(currSid.area, ',');
			// skip if area is inactive but set
			if (std::all_of(sidAreas.begin(),
							sidAreas.end(),
							[&](auto sidArea)
				{
					if (this->activeAirports[icao].areas.contains(sidArea) &&
						!this->activeAirports[icao].areas[sidArea].isActive
						) return true;
					else return false;
				}))
			{
				continue;
			}
			// skip if area is active + fpln outside or area is inactive
			if (std::none_of(sidAreas.begin(),
				sidAreas.end(),
				[&](auto sidArea)
				{
					if (this->activeAirports[icao].areas.contains(sidArea) &&
						!this->activeAirports[icao].areas[sidArea].isActive ||
						(this->activeAirports[icao].areas[sidArea].isActive &&
							!this->activeAirports[icao].areas[sidArea].inside(FlightPlan.GetFPTrackPosition().GetPosition()))
						) return false;
					else return true;
				}))
			{
				continue;
			}
		}
		// skip if lvp ops are active but SID is not configured for lvp ops and lvp is not disabled for SID
		if (this->activeAirports[fplnData.GetOrigin()].settings["lvp"] && !currSid.lvp && currSid.lvp != -1)
		{
			continue;
		}
		// skip if lvp ops are inactive but SID is configured for lvp ops
		if (!this->activeAirports[fplnData.GetOrigin()].settings["lvp"] && currSid.lvp && currSid.lvp != -1)
		{
			continue;
		}
		// skip if no matching rwy was found in SID;
		if (!rwyMatch) continue;
		// skip if engine type doesn't match
		if (currSid.engineType != "" && currSid.engineType.find(fplnData.GetEngineType()) == std::string::npos)
		{
			continue;
		}
		else if (currSid.engineType != "")
		{
			restriction = true;
		}
		// skip if an aircraft type is set in sid but is set to false
		if ((currSid.acftType.contains(fplnData.GetAircraftFPType()) &&
			!currSid.acftType[fplnData.GetAircraftFPType()]) ||
			std::any_of(currSid.acftType.begin(), currSid.acftType.end(), [&](auto type)
			{
				return type.second && !currSid.acftType.contains(fplnData.GetAircraftFPType());
			}))
		{
			//messageHandler->writeMessage("DEBUG", "Skipping due to acftType");
			continue;
		}
		else if (!currSid.acftType.empty())
		{
			restriction = true;
		}
		// skip if SID has engineNumber requirement and acft doesn't match
		if (!vsid::utils::containsDigit(currSid.engineCount, fplnData.GetEngineNumber()))
		{
			continue;
		}
		else if (currSid.engineCount > 0)
		{
			restriction = true;
		}
		// skip if SID has WTC requirement and acft doesn't match
		if (currSid.wtc != "" && currSid.wtc.find(fplnData.GetAircraftWtc()) == std::string::npos)
		{
			continue;
		}
		else if (currSid.wtc != "")
		{ 
			restriction = true; 
		}
		// skip if SID has mtow requirement and acft is too heavy - if grp config has not yet been loaded load it
		if (currSid.mtow) // IN DEVELOPMENT
		{
			if (this->configParser.grpConfig.size() == 0)
			{
				this->configParser.loadGrpConfig();
			}
			std::string acftType = fplnData.GetAircraftFPType();
			bool mtowMatch = false;
			for (auto it = this->configParser.grpConfig.begin(); it != this->configParser.grpConfig.end(); ++it)
			{
				if (it->value("ICAO", "") != acftType) continue;
				if (it->value("MTOW", 0) > currSid.mtow) break; // acft icao found but to heavy, no further checks
				mtowMatch = true;
				break; // acft light enough, no further checks
			}
			if (!mtowMatch) continue;
			else restriction = true;
		}
		// skip if sid has night times set but they're not active
		if (!actTSid.contains(currSid.waypoint) &&
			(currSid.timeFrom != -1 || currSid.timeTo != -1)
			) continue;
		// if a SID is accepted when filed by a pilot set the SID and break
		if (currSid.pilotfiled && currSid.name() == fplnData.GetSidName() && currSid.prio < prio)
		{
			setSid = currSid;
			prio = currSid.prio;
		}
		if (currSid.prio < prio && (setSid.pilotfiled == currSid.pilotfiled || restriction))
		{
			setSid = currSid;
			prio = currSid.prio;
		}
	}
	return(setSid);
}

void vsid::VSIDPlugin::processFlightplan(EuroScopePlugIn::CFlightPlan FlightPlan, bool checkOnly, std::string atcRwy, vsid::Sid manualSid)
{
	EuroScopePlugIn::CFlightPlan fpln = FlightPlan;
	EuroScopePlugIn::CFlightPlanData fplnData = fpln.GetFlightPlanData();
	EuroScopePlugIn::CFlightPlanControllerAssignedData cad = fpln.GetControllerAssignedData();
	std::string callsign = fpln.GetCallsign();
	std::string filedSidWpt = this->findSidWpt(fplnData);
	std::vector<std::string> filedRoute = vsid::utils::split(std::string(fplnData.GetRoute()), ' ');
	vsid::Sid sidSuggestion = {};
	vsid::Sid sidCustomSuggestion = {};
	//std::string sidByController;
	std::string setRwy = "";
	vsid::fpln::info fplnInfo = {};

	vsid::fpln::clean(filedRoute, fplnData.GetOrigin(), filedSidWpt);
	if (this->processed.contains(callsign))
	{
		fplnInfo.atcRWY = this->processed[callsign].atcRWY;
	}

	/* if a sid has been set manually choose this */
	if (std::string(FlightPlan.GetFlightPlanData().GetPlanType()) == "V")
	{
		if (!manualSid.empty())
		{
			sidCustomSuggestion = manualSid;
		}
	}
	else if (manualSid.waypoint != "")
	{
		sidSuggestion = this->processSid(fpln);
		sidCustomSuggestion = manualSid;

		if (sidSuggestion == sidCustomSuggestion)
		{
			sidCustomSuggestion = {};
		}
	}
	/* if a rwy is given by atc check for a sid for this rwy and for a normal sid
	* to be then able to compare those two
	*/
	else if (atcRwy != "")
	{
		sidSuggestion = this->processSid(fpln);
		sidCustomSuggestion = this->processSid(fpln, atcRwy);
	}
	/* default state */
	else
	{
		sidSuggestion = this->processSid(fpln);
	}

	if (sidSuggestion.waypoint != "" && sidCustomSuggestion.waypoint == "")
	{
		if (sidSuggestion.rwy.find(',') != std::string::npos)
		{
			std::vector<std::string> rwys = vsid::utils::split(sidSuggestion.rwy, ',');
			if (this->activeAirports[fplnData.GetOrigin()].depRwys.find(rwys.front()) !=
				this->activeAirports[fplnData.GetOrigin()].depRwys.end()
				)
			{
				setRwy = rwys.front();
			}
		}
		else
		{
			setRwy = sidSuggestion.rwy;
		}
	}
	else if (sidCustomSuggestion.waypoint != "")
	{
		if (sidCustomSuggestion.rwy.find(',') != std::string::npos)
		{
			if (atcRwy == "")
			{
				std::vector<std::string> rwySplit = vsid::utils::split(sidCustomSuggestion.rwy, ',');
				setRwy = rwySplit.front();
			}
			else setRwy = atcRwy;
		}
		else
		{
			setRwy = sidCustomSuggestion.rwy;
		}
	}
	
	// building a new route with the selected sid
	if (sidSuggestion.waypoint != "" && sidCustomSuggestion.waypoint == "")
	{
		std::ostringstream ss;
		ss << sidSuggestion.name() << "/" << setRwy;
		filedRoute.insert(filedRoute.begin(), vsid::utils::trim(ss.str()));
	}
	else if (sidCustomSuggestion.waypoint != "")
	{
		std::ostringstream ss;
		ss << sidCustomSuggestion.name() << "/" << setRwy;
		filedRoute.insert(filedRoute.begin(), vsid::utils::trim(ss.str()));
	}

	if (sidSuggestion.waypoint != "" && sidCustomSuggestion.waypoint == "")
	{
		fplnInfo.sid = sidSuggestion;
	}
	else if (sidCustomSuggestion.waypoint != "")
	{
		fplnInfo.sid = sidSuggestion;
		fplnInfo.customSid = sidCustomSuggestion;
	}
	this->processed[callsign] = fplnInfo;

	if(!checkOnly)
	{	
		this->processed[callsign].noFplnUpdate = true;
		if (!fplnData.SetRoute(vsid::utils::join(filedRoute).c_str()))
		{
			messageHandler->writeMessage("ERROR", "[" + callsign + "] - Failed to change Flighplan! - #PFP");
			this->processed[callsign].noFplnUpdate = false;
		}
		//else this->processed[callsign].noFplnUpdate = true;

		if (!fplnData.AmendFlightPlan())
		{
			messageHandler->writeMessage("ERROR", "[" + callsign + "] - Failed to amend Flighplan! - #PFP");
			this->processed[callsign].noFplnUpdate = false;
		}
		else
		{
			this->processed[callsign].atcRWY = true;
		}
		if (sidSuggestion.waypoint != "" && sidCustomSuggestion.waypoint == "" && sidSuggestion.initialClimb)
		{
			if (!cad.SetClearedAltitude(sidSuggestion.initialClimb))
			{
				messageHandler->writeMessage("ERROR", "[" + callsign + "] - failed to set altitude - #PFP");
			}
		}
		else if (sidCustomSuggestion.waypoint != "" && sidCustomSuggestion.initialClimb)
		{
			if (!cad.SetClearedAltitude(sidCustomSuggestion.initialClimb))
			{
				messageHandler->writeMessage("ERROR", "[" + callsign + "] - failed to set altitude - #PFP");
			}
		}
	}
}
/*
* END OWN FUNCTIONS
*/

/*
* BEGIN ES FUNCTIONS
*/

void vsid::VSIDPlugin::OnFunctionCall(int FunctionId, const char * sItemString, POINT Pt, RECT Area) {

	// if logged in as observer disable functions - DISABLED DURING DEVELOPMENT
	if (!ControllerMyself().IsController())
	{
		return;
	}

	EuroScopePlugIn::CFlightPlan fpln = FlightPlanSelectASEL();
	EuroScopePlugIn::CFlightPlanData fplnData = fpln.GetFlightPlanData();
	std::string callsign = fpln.GetCallsign();

	if (!fpln.IsValid()) return;

	/* DOCUMENTATION 
		//for (int i = 0; i < 8; i++) // test on how to get annotations
		//{
		//	std::string annotation = flightPlanASEL.GetControllerAssignedData().GetFlightStripAnnotation(i);
		//	vsid::messagehandler::LogMessage("Debug", "Annotation: " + annotation);
		//}

		delete FlightPlan;
	}*/

	// dynamically fill sid selection list with valid sids
	if (FunctionId == TAG_FUNC_VSID_SIDS_MAN)
	{
		std::string filedSidWpt = this->findSidWpt(fplnData);
		std::map<std::string, vsid::Sid> validDepartures;
		std::string depRWY = fplnData.GetDepartureRwy();

		for (vsid::Sid &sid : this->activeAirports[fplnData.GetOrigin()].sids)
		{
			if (sid.waypoint == filedSidWpt && sid.rwy.find(depRWY) != std::string::npos)
			{
				validDepartures[sid.waypoint + sid.number + sid.designator[0]] = sid;
				validDepartures[sid.waypoint + 'R' + 'V'] = vsid::Sid::Sid(sid.waypoint, 'R', "V", depRWY);
			}
			else if (filedSidWpt == "" && depRWY == "")
			{
				validDepartures[sid.waypoint + sid.number + sid.designator[0]] = sid;
				validDepartures[sid.waypoint + 'R' + 'V'] = vsid::Sid::Sid(sid.waypoint, 'R', "V", depRWY);
			}
			else if (filedSidWpt == "" && sid.rwy.find(depRWY) != std::string::npos &&
					this->activeAirports[fplnData.GetOrigin()].depRwys.find(depRWY) !=
					this->activeAirports[fplnData.GetOrigin()].depRwys.end()
					)
			{
				validDepartures[sid.waypoint + sid.number + sid.designator[0]] = sid;
				validDepartures[sid.waypoint + 'R' + 'V'] = vsid::Sid::Sid(sid.waypoint, 'R', "V", depRWY);
			}
		}

		if (strlen(sItemString) == 0)
		{
			this->OpenPopupList(Area, "Select SID", 1);
			if (validDepartures.size() == 0)
			{
				this->AddPopupListElement("NO SID", "NO SID", TAG_FUNC_VSID_SIDS_MAN, false, EuroScopePlugIn::POPUP_ELEMENT_NO_CHECKBOX, true, false);
			}
			for (const auto& sid : validDepartures)
			{
				this->AddPopupListElement(sid.first.c_str(), sid.first.c_str(), TAG_FUNC_VSID_SIDS_MAN, false, EuroScopePlugIn::POPUP_ELEMENT_NO_CHECKBOX, false, false);
			}
			if (std::string(fplnData.GetPlanType()) == "V")
			{
				this->AddPopupListElement("VFR", "VFR", TAG_FUNC_VSID_SIDS_MAN, false, EuroScopePlugIn::POPUP_ELEMENT_NO_CHECKBOX, false, true);
			}
		}
		if (std::string(sItemString) == "VFR")
		{
			std::vector<std::string> filedRoute = vsid::utils::split(fplnData.GetRoute(), ' ');
			vsid::fpln::clean(filedRoute, fplnData.GetOrigin(), filedSidWpt);

			if (depRWY != "")
			{
				std::ostringstream ss;
				ss << fplnData.GetOrigin() << "/" << depRWY;
				filedRoute.insert(filedRoute.begin(), ss.str());
			}
			this->processed[callsign].noFplnUpdate = true;
			if (!fplnData.SetRoute(vsid::utils::join(filedRoute).c_str()))
			{
				messageHandler->writeMessage("ERROR", "[" + std::string(fpln.GetCallsign()) + "] - Failed to change Flighplan! #MSS");
				this->processed[callsign].noFplnUpdate = false;
			}
			if (!fplnData.AmendFlightPlan())
			{
				messageHandler->writeMessage("ERROR", "[" + std::string(fpln.GetCallsign()) + "] - Failed to amend Flighplan! #MSS");
				this->processed[callsign].noFplnUpdate = false;
			}
			else
			{
				//this->processed[callsign].noFplnUpdate = true;
				this->processed[callsign].atcRWY = true;
			}
		}
		else if (strlen(sItemString) != 0 && std::string(sItemString) != "NO SID")
		{
			if (depRWY == "")
			{
				depRWY = validDepartures[sItemString].getRwy();
			}
			this->processFlightplan(fpln, false, depRWY, validDepartures[sItemString]);
		}
		// FlightPlan->flightPlan.GetControllerAssignedData().SetFlightStripAnnotation(0, sItemString) // test on how to set annotations (-> exclusive per plugin)
	}

	if (FunctionId == TAG_FUNC_VSID_SIDS_AUTO)
	{
		if (this->processed.find(fpln.GetCallsign()) != this->processed.end())
		{
			std::vector<std::string> filedRoute = vsid::utils::split(fplnData.GetRoute(), ' ');
			std::string atcRwy = vsid::fpln::getAtcBlock(filedRoute, fplnData.GetOrigin()).second;
			if (std::string(fplnData.GetPlanType()) == "I")
			{
				if (this->processed[callsign].atcRWY)
				{
					this->processFlightplan(fpln, false, atcRwy);
				}
				else this->processFlightplan(fpln, false);
			}
		}
	}

	if (FunctionId == TAG_FUNC_VSID_CLMBMENU)
	{
		std::map<std::string, int> alt;

		// code order important! the popuplist may only be generated when no sItemString is present (if it is a button has been clicked)
		// if the list gets set up again while clicking a button wrong values might occur
		for (int i = this->activeAirports[fplnData.GetOrigin()].maxInitialClimb; i >= vsid::utils::getMinClimb(this->activeAirports[fplnData.GetOrigin()].elevation); i -= 500)
		{
			std::string menuElem = (i > this->activeAirports[fplnData.GetOrigin()].transAlt) ? "0" + std::to_string(i / 100) : "A" + std::to_string(i / 100);
			alt[menuElem] = i;
		}

		if (strlen(sItemString) == 0)
		{
			this->OpenPopupList(Area, "Select Climb", 1);
			for (int i = this->activeAirports[fplnData.GetOrigin()].maxInitialClimb; i >= vsid::utils::getMinClimb(this->activeAirports[fplnData.GetOrigin()].elevation); i -= 500)
			{
				std::string clmbElem = (i > this->activeAirports[fplnData.GetOrigin()].transAlt) ? "0" + std::to_string(i / 100) : "A" + std::to_string(i / 100);
				this->AddPopupListElement(clmbElem.c_str(), clmbElem.c_str(), TAG_FUNC_VSID_CLMBMENU, false, EuroScopePlugIn::POPUP_ELEMENT_NO_CHECKBOX, false, false);
			}
		}
		if (strlen(sItemString) != 0)
		{
			if (!fpln.GetControllerAssignedData().SetClearedAltitude(alt[sItemString]))
			{
				messageHandler->writeMessage("ERROR", "Failed to set cleared altitude - #CLM");
			}
		}
	}

	if (FunctionId == TAG_FUNC_VSID_RWYMENU)
	{
		std::vector<std::string> allRwys = this->activeAirports[fplnData.GetOrigin()].allRwys;
		std::string rwy;

		if (strlen(sItemString) == 0)
		{
			this->OpenPopupList(Area, "RWY", 1);
			for (std::vector<std::string>::iterator it = allRwys.begin(); it != allRwys.end();)
			{
				rwy = *it;
				if (this->activeAirports[fplnData.GetOrigin()].depRwys.count(rwy) ||
					this->activeAirports[fplnData.GetOrigin()].arrRwys.count(rwy)
					)
				{
					this->AddPopupListElement(rwy.c_str(), rwy.c_str(), TAG_FUNC_VSID_RWYMENU, false, EuroScopePlugIn::POPUP_ELEMENT_NO_CHECKBOX, false, false);
					it = allRwys.erase(it);
				}
				else ++it;
			}
			if (allRwys.size() > 0)
			{
				for (std::vector<std::string>::iterator it = allRwys.begin(); it != allRwys.end(); ++it)
				{
					rwy = *it;
					this->AddPopupListElement(rwy.c_str(), rwy.c_str(), TAG_FUNC_VSID_RWYMENU, false, EuroScopePlugIn::POPUP_ELEMENT_NO_CHECKBOX, true, false);
				}
			}
		}
		if (strlen(sItemString) != 0)
		{
			std::vector<std::string> filedRoute = vsid::utils::split(fplnData.GetRoute(), ' ');
			vsid::fpln::clean(filedRoute, fplnData.GetOrigin());

			std::ostringstream ss;
			ss << fplnData.GetOrigin() << "/" << sItemString;
			filedRoute.insert(filedRoute.begin(), vsid::utils::trim(ss.str()));
			
			if (!fplnData.SetRoute(vsid::utils::join(filedRoute).c_str()))
			{
				messageHandler->writeMessage("ERROR", "[" + callsign + "] - Failed to change Flighplan! - #RM");
			}
			if (!vsid::fpln::findRemarks(fplnData, "VSID/RWY"))
			{
				this->processed[callsign].noFplnUpdate = true;
				if (!vsid::fpln::addRemark(fplnData, "VSID/RWY"))
				{
					messageHandler->writeMessage("ERROR", "[" + callsign + "] - Failed to set remarks! - #RM");
					this->processed[callsign].noFplnUpdate = false;
				}
			}
			if (!fplnData.AmendFlightPlan())
			{
				messageHandler->writeMessage("ERROR", "[" + callsign + "] - Failed to amend Flighplan! - #RM");
				this->processed[callsign].noFplnUpdate = false;
			}
			else if (this->activeAirports[fplnData.GetOrigin()].settings["auto"] && this->processed.contains(callsign))
			{
				this->processed.erase(callsign);
			}
			else
			{
				this->processed[callsign].atcRWY = true;
			}
		}
	}
}

void vsid::VSIDPlugin::OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize)
{
	std::string callsign = FlightPlan.GetCallsign();
	EuroScopePlugIn::CFlightPlanData fplnData = FlightPlan.GetFlightPlanData();

	// dev
	/*for (auto& area : this->activeAirports[fplnData.GetOrigin()].areas)
	{
		if (area.second.inside(FlightPlan.GetFPTrackPosition().GetPosition()))
		{
			messageHandler->writeMessage("DEBUG", callsign + " is in Area: " + area.first);
			break;
		}
	}*/
	// end dev

	if (!FlightPlan.IsValid())
	{
		return;
	}

	if (ItemCode == TAG_ITEM_VSID_SIDS)
	{
		*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
		std::pair<std::string, std::string> atcBlock = vsid::fpln::getAtcBlock(vsid::utils::split(fplnData.GetRoute(), ' '), fplnData.GetOrigin());

		if (this->removeProcessed.size() > 0 &&
			this->removeProcessed.contains(callsign) &&
			this->removeProcessed[callsign].second)
		{
			this->removeProcessed.erase(callsign);

			if (this->processed.contains(callsign))
			{
				if (atcBlock.first == "")
				{
					this->processed.erase(callsign);
				}
				EuroScopePlugIn::CFlightPlanControllerAssignedData cad = FlightPlan.GetControllerAssignedData();
				if (cad.GetClearedAltitude() == cad.GetFinalAltitude() &&
					atcBlock.first != "" &&
					ControllerMyself().IsController() &&
					(atcBlock.first == this->processed[callsign].sid.name() ||
					atcBlock.first == this->processed[callsign].customSid.name())
					)
				{
					if (!this->processed[callsign].customSid.empty())
					{
						if (!cad.SetClearedAltitude(this->processed[callsign].customSid.initialClimb))
						{
							messageHandler->writeMessage("ERROR", "[" + callsign + "] - failed to set altitude #RICS");
						}
					}
					else if (!this->processed[callsign].sid.empty())
					{
						if (!cad.SetClearedAltitude(this->processed[callsign].sid.initialClimb))
						{
							messageHandler->writeMessage("ERROR", "[" + callsign + "] - failed to set altitude #RICS");
						}
					}
				}
			}
		}

		if (this->processed.contains(callsign))
		{
			std::string sidName = this->processed[callsign].sid.name();
			std::string customSidName = this->processed[callsign].customSid.name();

			if (((atcBlock.first == fplnData.GetOrigin() &&
				this->processed[callsign].atcRWY &&
				this->processed[callsign].customSid.empty()) ||
				(atcBlock.first == "" &&
				this->processed[callsign].sid.empty() &&
				this->processed[callsign].customSid.empty())) &&
				std::string(FlightPlan.GetFlightPlanData().GetPlanType()) == "I"
				)
			{
				*pRGB = this->configParser.getColor("noSid");
			}
			else if ((atcBlock.first == "" ||
					atcBlock.first == fplnData.GetOrigin()) &&
					(this->processed[callsign].customSid.empty() ||
					this->processed[callsign].sid == this->processed[callsign].customSid) ||
					(std::string(FlightPlan.GetFlightPlanData().GetPlanType()) == "V" &&
					std::string(sItemString) == "VFR"
					)
					)
			{
				*pRGB = this->configParser.getColor("sidSuggestion");
			}
			else if (atcBlock.first != "" &&
					atcBlock.first == fplnData.GetOrigin() &&
					!this->processed[callsign].customSid.empty() &&
					this->processed[callsign].sid != this->processed[callsign].customSid
					)
			{
				*pRGB = this->configParser.getColor("customSidSuggestion");
			}
			else if ((atcBlock.first != "" &&
					atcBlock.first != fplnData.GetOrigin() &&
					atcBlock.first == customSidName &&
					this->processed[callsign].sid != this->processed[callsign].customSid) ||
					atcBlock.first != sidName
					)
			{
				*pRGB = this->configParser.getColor("customSidSet");
			}
			else if (atcBlock.first != "" && atcBlock.first == sidName)
			{
				*pRGB = this->configParser.getColor("suggestedSidSet");
			}
			else
			{
				*pRGB = RGB(140, 140, 60);
			}

			if (atcBlock.first != "" && atcBlock.first != fplnData.GetOrigin())
			{
				strcpy_s(sItemString, 16, atcBlock.first.c_str());
			}
			else if ((atcBlock.first == "" ||
					atcBlock.first == fplnData.GetOrigin()) &&
					std::string(FlightPlan.GetFlightPlanData().GetPlanType()) == "V"
					)
			{
				strcpy_s(sItemString, 16, "VFR");
			}
			else if ((atcBlock.first != "" &&
					atcBlock.first == fplnData.GetOrigin() &&
					this->processed[callsign].atcRWY &&
					this->processed[callsign].customSid.rwy.find(atcBlock.second) == std::string::npos) ||
					(this->processed[callsign].sid.empty() &&
					this->processed[callsign].customSid.empty())
					)
			{
				strcpy_s(sItemString, 16, "MANUAL");
			}
			else if(sidName != "" && customSidName == "")
			{
				strcpy_s(sItemString, 16, sidName.c_str());
			}
			else if (customSidName != "")
			{
				strcpy_s(sItemString, 16, customSidName.c_str());
			}
		}
		else if(this->activeAirports.contains(fplnData.GetOrigin()) && RadarTarget.GetGS() <= 50 )
		{
			bool checkOnly = !this->activeAirports[fplnData.GetOrigin()].settings["auto"];

			// suspend flightplan editing after config reloading
			if (!checkOnly)
			{
				if (FlightPlan.GetClearenceFlag() ||
					fplnData.GetPlanType() == "V"/* ||
					(atcBlock.first != "" && atcBlock.first != fplnData.GetOrigin())*/
					)
				{
					checkOnly = true;
				}
			}
			if (atcBlock.second != "" &&
				(vsid::fpln::findRemarks(fplnData, "VSID/RWY") ||
				atcBlock.first != fplnData.GetOrigin() ||
				fplnData.IsAmended())
				)
			{
				this->processFlightplan(FlightPlan, checkOnly, atcBlock.second);
			}
			else
			{
				this->processFlightplan(FlightPlan, checkOnly);
			}
			
		}
	}

	if (ItemCode == TAG_ITEM_VSID_CLIMB)
	{
		*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
		EuroScopePlugIn::CFlightPlan fpln = FlightPlan;

		int transAlt = this->activeAirports[fplnData.GetOrigin()].transAlt;
		int tempAlt = 0;
		int climbVia = 0;

		if (this->processed.find(callsign) != this->processed.end())
		{
			// determine if a found non-standard sid exists in valid sids and set it for ic comparison

			std::string sidName = this->processed[callsign].sid.name();
			std::string customSidName = this->processed[callsign].customSid.name();
			std::string atcSid = vsid::fpln::getAtcBlock(vsid::utils::split(fplnData.GetRoute(), ' '), fplnData.GetOrigin()).first;
			
			// if an unknown Sid is set (non-standard or non-custom) try to find matching Sid in config
			if (atcSid != "" && atcSid != fplnData.GetOrigin() && atcSid != sidName && atcSid != customSidName)
			{
				for (vsid::Sid& sid : this->activeAirports[fplnData.GetOrigin()].sids)
				{
					if (sid.waypoint != atcSid.substr(0, atcSid.length() - 2)) continue;
					if (sid.designator[0] != atcSid[atcSid.length() - 1]) continue;
					if (sid.number != atcSid[atcSid.length() - 2]) break;
					
					this->processed[callsign].customSid = sid;
				}
			}

			// determine if climb via is needed depending on customSid
			if (atcSid == "" || atcSid == sidName || atcSid == fplnData.GetOrigin() && sidName != customSidName)
			{
				climbVia = this->processed[callsign].sid.climbvia;
			}
			else if (atcSid == "" || atcSid == customSidName || atcSid == fplnData.GetOrigin())
			{
				climbVia = this->processed[callsign].customSid.climbvia;
			}

			// determine initial climb depending on customSid

			if (this->processed[callsign].sid.initialClimb != 0 &&
				this->processed[callsign].customSid.empty() &&
				(atcSid == sidName || atcSid == "" || atcSid == fplnData.GetOrigin())
				)
			{
				tempAlt = this->processed[callsign].sid.initialClimb;
			}
			else if (this->processed[callsign].customSid.initialClimb != 0 &&
					(atcSid == customSidName || atcSid == "" || atcSid == fplnData.GetOrigin())
					)
			{
				tempAlt = this->processed[callsign].customSid.initialClimb;
			}

			if (fpln.GetClearedAltitude() == fpln.GetFinalAltitude())
			{
				*pRGB = this->configParser.getColor("suggestedClmb"); // white
			}
			else if (fpln.GetClearedAltitude() != fpln.GetFinalAltitude() &&
					fpln.GetClearedAltitude() == tempAlt
					)
			{
				if (climbVia)
				{
					*pRGB = this->configParser.getColor("clmbViaSet"); // green
				}
				else
				{
					*pRGB = this->configParser.getColor("clmbSet"); // cyan
				}
			}
			else
			{
				*pRGB = this->configParser.getColor("customClmbSet"); // orange
			}

			// determine the initial climb depending on existing customSid

			if (fpln.GetClearedAltitude() == fpln.GetFinalAltitude())
			{
				if (tempAlt == 0)
				{
					strcpy_s(sItemString, 16, std::string("---").c_str());
				}
				else if (tempAlt <= transAlt)
				{
					strcpy_s(sItemString, 16, std::string("A").append(std::to_string(tempAlt / 100)).c_str());
				}
				else
				{
					if (tempAlt / 100 >= 100)
					{
						strcpy_s(sItemString, 16, std::to_string(tempAlt / 100).c_str());
					}
					else
					{
						strcpy_s(sItemString, 16, std::string("0").append(std::to_string(tempAlt / 100)).c_str());
					}
				}
			}
			else
			{
				if(fpln.GetClearedAltitude() == 0)
				{
					strcpy_s(sItemString, 16, std::string("---").c_str());
				}
				else if (fpln.GetClearedAltitude() <= transAlt)
				{
					strcpy_s(sItemString, 16, std::string("A").append(std::to_string(fpln.GetClearedAltitude() / 100)).c_str());
				}
				else
				{
					if (fpln.GetClearedAltitude() / 100 >= 100)
					{
						strcpy_s(sItemString, 16, std::to_string(fpln.GetClearedAltitude() / 100).c_str());
					}
					else
					{
						strcpy_s(sItemString, 16, std::string("0").append(std::to_string(fpln.GetClearedAltitude() / 100)).c_str());
					}
				}
			}			
		}
	}

	if (ItemCode == TAG_ITEM_VSID_RWY)
	{
		*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;

		if (this->processed.find(callsign) != this->processed.end())
		{
			//std::vector<std::string> filedRoute = vsid::utils::split(fplnData.GetRoute(), ' ');
			std::pair<std::string, std::string> atcBlock = vsid::fpln::getAtcBlock(
																		vsid::utils::split(fplnData.GetRoute(), ' '),
																		fplnData.GetOrigin()
																		);

			if (atcBlock.first == fplnData.GetOrigin() &&
				!this->processed[callsign].atcRWY &&
				!this->processed[callsign].remarkChecked
				)
			{
				this->processed[callsign].atcRWY = vsid::fpln::findRemarks(fplnData, "VSID/RWY");
				this->processed[callsign].remarkChecked = true;
			}
			else if (atcBlock.first == fplnData.GetOrigin() &&
				!this->processed[callsign].atcRWY &&
				fplnData.IsAmended()
				)
			{
				this->processed[callsign].atcRWY = true;
			}
			else if (!this->processed[callsign].atcRWY &&
				(atcBlock.first == this->processed[callsign].sid.name() ||
				atcBlock.first == this->processed[callsign].customSid.name())
				)
			{
				this->processed[callsign].atcRWY = true;
			}
			else if (!this->processed[callsign].atcRWY && // MONITOR
					atcBlock.first != fplnData.GetOrigin() &&
					(fplnData.IsAmended() || FlightPlan.GetClearenceFlag())
					)
			{
				this->processed[callsign].atcRWY = true;
			}

			if (atcBlock.second != "" &&
				this->activeAirports[fplnData.GetOrigin()].depRwys.contains(atcBlock.second) &&
				this->processed[callsign].atcRWY
				)
			{
				*pRGB = this->configParser.getColor("rwySet");
			}
			else if (atcBlock.second != "" &&
				!this->activeAirports[fplnData.GetOrigin()].depRwys.contains(atcBlock.second) &&
				this->processed[callsign].atcRWY
				)
			{
				*pRGB = this->configParser.getColor("notDepRwySet");
			}
			else *pRGB = this->configParser.getColor("rwyNotSet");

			if (atcBlock.second != "" && this->processed[callsign].atcRWY)
			{
				strcpy_s(sItemString, 16, atcBlock.second.c_str());
			}
			else
			{
				std::string sidRwy;
				if (this->processed[callsign].sid.rwy.find(',') != std::string::npos)
				{
					sidRwy = vsid::utils::split(this->processed[callsign].sid.rwy, ',').front();
				}
				else
				{
					sidRwy = this->processed[callsign].sid.rwy;
				}
				if (sidRwy == "" && std::string(fplnData.GetDepartureRwy()) != "")
				{
					sidRwy = fplnData.GetDepartureRwy();
				}
				strcpy_s(sItemString, 16, sidRwy.c_str());
			}
		}
		else
		{
			*pRGB = this->configParser.getColor("rwyNotSet");
			strcpy_s(sItemString, 16, fplnData.GetDepartureRwy());
		}
	}
}

bool vsid::VSIDPlugin::OnCompileCommand(const char* sCommandLine)
{
	std::vector<std::string> command = vsid::utils::split(sCommandLine, ' ');

	if (command[0] == ".vsid")
	{
		if (command.size() == 1)
		{
			messageHandler->writeMessage("INFO", "Available commands: "
				"version / "
				"Status - (inop) / "
				"Level - (inop) / "
				"auto [icao] - activate automode for icao (inop) /"
				"area [icao] - toggle area for icao /"
				"rule [icao] [rulename] - toggle rule for icao / "
				"night [icao] - toggle night mode for icao /"
				"lvp [icao] - toggle lvp ops for icao / "
				"Debug - toggle debug mode (inop)");
			return true;
		}
		if (vsid::utils::tolower(command[1]) == "version")
		{
			messageHandler->writeMessage("INFO", "vSID Version " + pluginVersion + " loaded.");
			return true;
		}
		// dev
		else if (vsid::utils::tolower(command[1]) == "removed")
		{
			for (auto &elem : this->removeProcessed)
			{
				messageHandler->writeMessage("DEBUG", elem.first + " being removed at: " + vsid::time::toString(elem.second.first) + " and is disconnected " + ((elem.second.second) ? "YES" : "NO"));
			}
			if (this->removeProcessed.size() == 0)
			{
				messageHandler->writeMessage("DEBUG", "Removed list empty");
			}
			return true;
		}
		// end dev
		else if (vsid::utils::tolower(command[1]) == "rule")
		{
			if (command.size() == 2)
			{
				for (std::pair<const std::string, vsid::Airport>& airport : this->activeAirports)
				{
					if (!airport.second.customRules.empty())
					{
						std::ostringstream ss;
						for (std::pair<const std::string, bool>& rule : airport.second.customRules)
						{
							std::string status = (rule.second) ? "ON" : "OFF";
							ss << rule.first << ": " << status << " ";
						}
						messageHandler->writeMessage(airport.first + " Rules", ss.str());
					}
					else messageHandler->writeMessage(airport.first + " Rules", "No rules configured");
				}
			}
			else if (command.size() >= 3 && this->activeAirports.contains(vsid::utils::toupper(command[2])))
			{
				std::string icao = vsid::utils::toupper(command[2]);
				if (command.size() == 3)
				{
					if (!this->activeAirports[icao].customRules.empty())
					{
						std::ostringstream ss;
						for (std::pair<const std::string, bool>& rule : this->activeAirports[icao].customRules)
						{
							std::string status = (rule.second) ? "ON" : "OFF";
							ss << rule.first << ": " << status;
						}
						messageHandler->writeMessage(icao + " Rules", ss.str());
					}
					else messageHandler->writeMessage(icao + " Rules", "No rules configured");
				}
				else if (command.size() == 4)
				{
					std::string rule = vsid::utils::toupper(command[3]);
					if (this->activeAirports[icao].customRules.contains(rule))
					{
						if (this->activeAirports[icao].customRules[rule])
						{
							this->activeAirports[icao].customRules[rule] = false;
							messageHandler->writeMessage(icao + " Rule", rule + " " + (this->activeAirports[icao].customRules[rule] ? "ON" : "OFF"));
						}
						else
						{
							this->activeAirports[icao].customRules[rule] = true;
							messageHandler->writeMessage(icao + " Rule", rule + " " + (this->activeAirports[icao].customRules[rule] ? "ON" : "OFF"));
						}
						this->UpdateActiveAirports();
					}
					else messageHandler->writeMessage(icao + " " + command[3], "Rule is unknown");
				}
			}
			else messageHandler->writeMessage("INFO", vsid::utils::toupper(command[2]) + " not in active airports");
			return true;
		}
		else if (vsid::utils::tolower(command[1]) == "lvp")
		{
			if (command.size() == 2)
			{
				std::ostringstream ss;
				for (auto it = this->activeAirports.begin(); it != this->activeAirports.end();)
				{
					std::string status = (it->second.settings["lvp"]) ? "ON" : "OFF";

					ss << it->first << " LVP: " << status;
					it++;
					if (it != this->activeAirports.end()) ss << " / ";
				}
				messageHandler->writeMessage("INFO", ss.str());
			}
			else if (command.size() == 3)
			{
				if (this->activeAirports.count(vsid::utils::toupper(command[2])))
				{
					if (this->activeAirports[vsid::utils::toupper(command[2])].settings["lvp"])
					{
						this->activeAirports[vsid::utils::toupper(command[2])].settings["lvp"] = 0;
						messageHandler->writeMessage("INFO", vsid::utils::toupper(command[2]) + " LVP: " +
							((this->activeAirports[vsid::utils::toupper(command[2])].settings["lvp"]) ? "ON" : "OFF")
						);
					}
					else
					{
						this->activeAirports[vsid::utils::toupper(command[2])].settings["lvp"] = 1;
						messageHandler->writeMessage("INFO", vsid::utils::toupper(command[2]) + " LVP: " +
							((this->activeAirports[vsid::utils::toupper(command[2])].settings["lvp"]) ? "ON" : "OFF")
						);
					}
					this->UpdateActiveAirports();
				}
				else messageHandler->writeMessage("INFO", vsid::utils::toupper(command[2]) + " is not in active airports");
			}
			return true;
		}
		else if (vsid::utils::tolower(command[1]) == "night" || vsid::utils::tolower(command[1]) == "time")
		{
			if (command.size() == 2)
			{
				std::ostringstream ss;
				for (auto it = this->activeAirports.begin(); it != this->activeAirports.end();)
				{
					std::string status = (it->second.settings["time"]) ? "ON" : "OFF";

					ss << it->first << " Time Mode: " << status;
					it++;
					if (it != this->activeAirports.end()) ss << " / ";
				}
				messageHandler->writeMessage("INFO", ss.str());
			}
			else if (command.size() == 3)
			{
				if (this->activeAirports.count(vsid::utils::toupper(command[2])))
				{
					if (this->activeAirports[vsid::utils::toupper(command[2])].settings["time"])
					{
						this->activeAirports[vsid::utils::toupper(command[2])].settings["time"] = 0;
					}
					else
					{
						this->activeAirports[vsid::utils::toupper(command[2])].settings["time"] = 1;
					}
					messageHandler->writeMessage("INFO", vsid::utils::toupper(command[2]) + " Time Mode: " +
						((this->activeAirports[vsid::utils::toupper(command[2])].settings["time"]) ? "ON" : "OFF")
					);
					this->UpdateActiveAirports();
				}
				else messageHandler->writeMessage("INFO", vsid::utils::toupper(command[2]) + " is not in active airports");
			}
			return true;
		}
		else if (vsid::utils::tolower(command[1]) == "auto")
		{
			
			if (!ControllerMyself().IsController())
			{
				messageHandler->writeMessage("INFO", "Automode not available for observer");
				return true;
			}
			if (command.size() == 2)
			{
				int counter = 0;
				std::ostringstream ss;
				ss << "Automode ON for: ";
				for (auto it = this->activeAirports.begin(); it != this->activeAirports.end(); ++it)
				{
					if (!it->second.settings["auto"] && it->second.controllers.size() == 0)
					{
						it->second.settings["auto"] = true;
						ss << it->first << " ";
						counter++;
					}
					else if (!it->second.settings["auto"] && it->second.controllers.size() > 0)
					{
						if (std::all_of(it->second.controllers.begin(), it->second.controllers.end(), [&](auto controller)
							{
								return controller.second.facility > ControllerMyself().GetFacility();
							}))
						{
							it->second.settings["auto"] = true;
							ss << it->first << " ";
							counter++;
						}
						else if (ControllerMyself().GetFacility() >= 6 &&
							std::none_of(it->second.controllers.begin(), it->second.controllers.end(), [&](auto controller)
								{

									if ((it->second.appSI.contains(ControllerMyself().GetPositionId()) &&
										it->second.appSI.contains(controller.second.si) &&
										it->second.appSI[ControllerMyself().GetPositionId()] > it->second.appSI[controller.second.si]) ||
										(!it->second.appSI.contains(ControllerMyself().GetPositionId()) &&
										it->second.appSI.contains(controller.second.si))
										) return true;
									else return false;
								}))
						{
							it->second.settings["auto"] = true;
							ss << it->first << " ";
							counter++;
						}
						else
						{
							messageHandler->writeMessage("INFO", "Cannot activate automode for " + it->first + ". Lower controller online");
							continue;
						}
					}
				}

				if (counter > 0) messageHandler->writeMessage("INFO", ss.str());
				else messageHandler->writeMessage("INFO", "No new automode. Check .vsid auto status for active ones.");
				
				auto count = std::erase_if(this->processed, [&](auto item) 
					{
						EuroScopePlugIn::CFlightPlan fpln = FlightPlanSelect(item.first.c_str());
						EuroScopePlugIn::CFlightPlanData fplnData = fpln.GetFlightPlanData();
						if (!fpln.GetClearenceFlag() &&
							this->activeAirports.contains(fplnData.GetOrigin()) &&
							this->activeAirports[fplnData.GetOrigin()].settings["auto"]
							)
						{
							return true;
						}
						else return false;
					}
				);
			}
			else if (command.size() > 2 && vsid::utils::tolower(command[2]) == "status")
			{
				std::ostringstream ss;
				ss << "Automode active for: ";
				int counter = 0;
				for (auto it = this->activeAirports.begin(); it != this->activeAirports.end(); ++it)
				{
					if (it->second.settings["auto"])
					{
						ss << it->first << " ";
						counter++;
					}
				}
				if (counter > 0) messageHandler->writeMessage("INFO", ss.str());
				else messageHandler->writeMessage("INFO", "No active automode.");
			}
			else if (command.size() > 2 && vsid::utils::tolower(command[2]) == "off")
			{
				for (auto it = this->activeAirports.begin(); it != this->activeAirports.end(); ++it)
				{
					it->second.settings["auto"] = false;
				}
				messageHandler->writeMessage("INFO", "Automode OFF for all airports.");
			}
			else if (command.size() > 2)
			{
				for (std::vector<std::string>::iterator it = command.begin() + 2; it != command.end(); ++it)
				{
					if (this->activeAirports.contains(*it))
					{
						this->activeAirports[*it].settings["auto"] = !this->activeAirports[*it].settings["auto"];
						messageHandler->writeMessage("INFO", *it + " automode is: " + ((this->activeAirports[*it].settings["auto"]) ? "ON" : "OFF"));
						if (this->activeAirports[*it].settings["auto"])
						{
							this->activeAirports[*it].forceAuto = true;
						}
						else this->activeAirports[*it].forceAuto = false;
					}
					else messageHandler->writeMessage("INFO", *it + " not in active airports. Cannot set automode");
				}
			}
			return true;
		}
		else if (vsid::utils::tolower(command[1]) == "area")
		{
			if (command.size() == 2)
			{
				for (std::pair<const std::string, vsid::Airport>& apt : this->activeAirports)
				{
					if (!apt.second.areas.empty())
					{
						std::ostringstream ss;
						for (std::pair<const std::string, vsid::Area>& area : apt.second.areas)
						{
							std::string status = (area.second.isActive) ? "ON" : "OFF";
							ss << area.first << ": " << status << " ";
						}
						messageHandler->writeMessage(apt.first + " Areas", ss.str());
					}
					else messageHandler->writeMessage(apt.first + " Areas", "No area settings configured");
				}
			}
			else if (command.size() >= 3 && this->activeAirports.contains(vsid::utils::toupper(command[2])))
			{
				std::string icao = vsid::utils::toupper(command[2]);
				if (command.size() == 3)
				{
					if (!this->activeAirports[icao].areas.empty())
					{
						std::ostringstream ss;
						for (std::pair<const std::string, vsid::Area>& area : this->activeAirports[icao].areas)
						{
							std::string status = (area.second.isActive) ? "ON " : "OFF ";
							ss << area.first << ": " << status;
						}
						messageHandler->writeMessage(icao + " Areas", ss.str());
					}
					else messageHandler->writeMessage(icao + " Areas", "No area settings configured");
				}
				else if (command.size() == 4)
				{
					std::string area = vsid::utils::toupper(command[3]);
					if (!this->activeAirports[icao].areas.empty() && area == "ALL")
					{
						std::ostringstream ss;
						for (std::pair<const std::string, vsid::Area> &el : this->activeAirports[icao].areas)
						{
							el.second.isActive = true;
							ss << el.first << ": " << ((el.second.isActive) ? "ON " : "OFF ");
						}
						messageHandler->writeMessage(icao + " Areas", ss.str());
					}
					else if (!this->activeAirports[icao].areas.empty() && area == "OFF")
					{
						std::ostringstream ss;
						for (std::pair<const std::string, vsid::Area>& el : this->activeAirports[icao].areas)
						{
							el.second.isActive = false;
							ss << el.first << ": " << ((el.second.isActive) ? "ON " : "OFF ");
						}
						messageHandler->writeMessage(icao + " Areas", ss.str());
					}
					else if (this->activeAirports[icao].areas.contains(area))
					{
						this->activeAirports[icao].areas[area].isActive = !this->activeAirports[icao].areas[area].isActive;
						messageHandler->writeMessage(icao + " Area", area + " " +
													(this->activeAirports[icao].areas[area].isActive ? "ON" : "OFF")
													);
					}
					else messageHandler->writeMessage(icao + " " + command[3], "Area is unknown");
					
					auto count = std::erase_if(this->processed, [&](auto item)
						{
							EuroScopePlugIn::CFlightPlan fpln = FlightPlanSelect(item.first.c_str());
							EuroScopePlugIn::CFlightPlanData fplnData = fpln.GetFlightPlanData();
							if (!fpln.GetClearenceFlag() &&
								this->activeAirports.contains(fplnData.GetOrigin()) /* &&
								this->activeAirports[fplnData.GetOrigin()].settings["auto"]*/
								)
							{
								return true;
							}
							else return false;
						}
					);
				}
			}
			else messageHandler->writeMessage("INFO", vsid::utils::toupper(command[2]) + " not in active airports");
			return true;
		}
		else if (vsid::utils::tolower(command[1]) == "debug")
		{
			if (this->debug == false)
			{
				this->debug = true;
				//vsid::messagehandler::LogMessage("Debug", "DEBUG MODE: ON");
				messageHandler->writeMessage("Debug", "DEBUG MODE: ON");
				return true;
			}
			else
			{
				//vsid::messagehandler::LogMessage("Debug", "DEBUG MODE: OFF");
				messageHandler->writeMessage("Debug", "DEBUG MODE: OFF");
				this->debug = false;
				return true;
			}
		}
		else
		{
			messageHandler->writeMessage("INFO", "Unknown command");
			return true;
		}
	}
	return false;
}

void vsid::VSIDPlugin::OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan)
{
	// if we receive updates for flightplans validate entered sid again to sync between controllers
	// updates are received for any flightplan not just that under control

	EuroScopePlugIn::CFlightPlanData fplnData = FlightPlan.GetFlightPlanData();
	std::string callsign = FlightPlan.GetCallsign();

	if (!FlightPlan.IsValid())
	{
		return;
	}
	if (this->processed.contains(callsign))
	{
		std::vector<std::string> filedRoute = vsid::utils::split(fplnData.GetRoute(), ' ');
		if (filedRoute.size() > 0)
		{
			std::pair<std::string, std::string> atcBlock = vsid::fpln::getAtcBlock(filedRoute, fplnData.GetOrigin());
			if (this->activeAirports[fplnData.GetOrigin()].settings["auto"] &&
				atcBlock.first == "")
			{
				this->processed.erase(callsign);
				return;
			}

			if (!this->processed[callsign].atcRWY &&
				(fplnData.IsAmended() || FlightPlan.GetClearenceFlag() ||
				atcBlock.first != fplnData.GetOrigin() ||
				(atcBlock.first == fplnData.GetOrigin() && vsid::fpln::findRemarks(fplnData, "VSID/RWY")))
				)
			{
				this->processed[callsign].atcRWY = true;
			}

			if (vsid::fpln::findRemarks(fplnData, "VSID/RWY") &&
				atcBlock.first != fplnData.GetOrigin() &&
				ControllerMyself().IsController()
				)
			{
				this->processed[callsign].noFplnUpdate = true;
				if (!vsid::fpln::removeRemark(fplnData, "VSID/RWY"))
				{
					messageHandler->writeMessage("ERROR", "[" + callsign + "] - Failed to remove remarks! - #FFDU");
				}
				else this->processed[callsign].noFplnUpdate = false;
				if (!fplnData.AmendFlightPlan())
				{
					messageHandler->writeMessage("ERROR", "[" + callsign + "] - Failed to amend Flighplan! - #FFDU");
				}
				else this->processed[callsign].noFplnUpdate = false;
			}

			// if we want to supress an update that happened and skip sid checking  /////////////// needs further checking - this the right position? now several flightplanupdates again + auto mode ignores set custom rwy when activated
			/*if (this->processed[callsign].noFplnUpdate)
			{
				messageHandler->writeMessage("DEBUG", callsign + " nofplnUpdate true. Disabling.");
				this->processed[callsign].noFplnUpdate = false;
				messageHandler->writeMessage("DEBUG", callsign + " nofplnUpdate after disabling " + ((this->processed[callsign].noFplnUpdate) ? "TRUE" : "FALSE"));
				return;
			}*/

			if (atcBlock.first == fplnData.GetOrigin() && atcBlock.second != "")
			{
				this->processFlightplan(FlightPlan, true, atcBlock.second);
			}
			else if (atcBlock.first != fplnData.GetOrigin() && atcBlock.second != "")
			{
				vsid::Sid atcSid;
				for (vsid::Sid& sid : this->activeAirports[fplnData.GetOrigin()].sids)
				{
					if (sid.waypoint != atcBlock.first.substr(0, atcBlock.first.length() - 2)) continue;
					if (sid.designator[0] != atcBlock.first[atcBlock.first.length() - 1]) continue;
					atcSid = sid;
				}
				this->processFlightplan(FlightPlan, true, atcBlock.second, atcSid);
			}
			else
			{
				this->processFlightplan(FlightPlan, true);
			}
		}
	}
}

void vsid::VSIDPlugin::OnFlightPlanDisconnect(EuroScopePlugIn::CFlightPlan FlightPlan)
{
	if (this->processed.contains(FlightPlan.GetCallsign()))
	{
		std::string callsign = FlightPlan.GetCallsign();
		auto now = std::chrono::utc_clock::now() + std::chrono::minutes{ 1 };
		this->removeProcessed[callsign] = { now, true };
	}
}

void vsid::VSIDPlugin::OnRadarTargetPositionUpdate(EuroScopePlugIn::CRadarTarget RadarTarget)
{
	std::string callsign = RadarTarget.GetCallsign();
	if (this->processed.contains(RadarTarget.GetCallsign()) &&
		RadarTarget.GetGS() >= 50 &&
		!this->removeProcessed.contains(callsign)
		)
	{
		auto now = std::chrono::utc_clock::now() + std::chrono::minutes{ 10 };
		this->removeProcessed[callsign] = { now, false };
	}
}

void vsid::VSIDPlugin::OnControllerPositionUpdate(EuroScopePlugIn::CController Controller)
{
	std::string atcCallsign = Controller.GetCallsign();
	std::string atcSI = Controller.GetPositionId();
	int atcFac = Controller.GetFacility();
	double atcFreq = Controller.GetPrimaryFrequency();
	std::string atcIcao = "";
	EuroScopePlugIn::CController atcMyself = ControllerMyself();

	if (!Controller.IsController() ||
		atcCallsign == ControllerMyself().GetCallsign() ||
		atcCallsign.find("ATIS") != std::string::npos ||
		atcSI.find_first_of("0123456789") != std::string::npos ||
		atcFac < 2 ||
		//atcFac > ControllerMyself().GetFacility() ||
		atcFreq == 0.0 ||
		atcFreq > 199.0 ||
		this->actAtc.contains(atcSI) ||
		this->ignoreAtc.contains(atcSI)
		)
	{
		return;
	}

	if (atcFac < 6)
	{
		atcIcao = vsid::utils::split(Controller.GetCallsign(), '_').front();
	}

	if (atcFac >= 6 && !this->actAtc.contains(atcSI) && !this->ignoreAtc.contains(atcSI))
	{
		bool ignore = true;
		for (std::pair<const std::string, vsid::Airport> &apt : this->activeAirports)
		{
			if (apt.second.appSI.contains(atcSI))
			{
				ignore = false;
				atcIcao = apt.first;
			}
		}
		if (ignore)
		{
			this->ignoreAtc.insert(atcSI);
			return;
		}
	}

	if (!this->activeAirports.contains(atcIcao)) return;

	if (!this->activeAirports[atcIcao].controllers.contains(atcSI))
	{
		this->activeAirports[atcIcao].controllers[atcSI] = { atcSI, atcFac, atcFreq };
		this->actAtc[atcSI] = atcIcao;
	}

	if (this->activeAirports[atcIcao].settings["auto"] &&
		!this->activeAirports[atcIcao].forceAuto &&
		(
			(atcFac < 6 && atcFac < atcMyself.GetFacility()) ||
			(atcFac >= 6 &&
				(this->activeAirports[atcIcao].appSI.contains(atcMyself.GetPositionId()) &&
				this->activeAirports[atcIcao].appSI.contains(atcSI) &&
				this->activeAirports[atcIcao].appSI[atcMyself.GetPositionId()] > this->activeAirports[atcIcao].appSI[atcSI]) ||
				this->activeAirports[atcIcao].appSI.contains(atcSI)
			)
		)
	)
	{
		this->activeAirports[atcIcao].settings["auto"] = false;
		messageHandler->writeMessage("INFO", "Disabling auto mode for " +
									atcIcao + ". " + atcCallsign + " now online."
									);
	}
}

void vsid::VSIDPlugin::OnControllerDisconnect(EuroScopePlugIn::CController Controller) // use this to check for lower atc disconnects to start automation again
{
	std::string atcSI = Controller.GetPositionId();
	if (this->actAtc.contains(atcSI))
	{
		if (this->activeAirports[this->actAtc[atcSI]].controllers.contains(atcSI))
		{
			this->activeAirports[this->actAtc[atcSI]].controllers.erase(atcSI);
		}
		this->actAtc.erase(atcSI);
	}

	if (this->ignoreAtc.contains(atcSI))
	{
		this->ignoreAtc.erase(atcSI);
	}
}

void vsid::VSIDPlugin::OnAirportRunwayActivityChanged()
{
	this->UpdateActiveAirports();
}

void vsid::VSIDPlugin::UpdateActiveAirports()
{
	messageHandler->writeMessage("INFO", "Updating airports...");
	this->savedSettings.clear();
	for (std::pair<const std::string, vsid::Airport>& apt : this->activeAirports)
	{
		this->savedSettings.insert({ apt.first, apt.second.settings });
		this->savedRules.insert({ apt.first, apt.second.customRules });
		this->savedAreas.insert({ apt.first, apt.second.areas });
	}

	this->SelectActiveSectorfile();
	this->activeAirports.clear();
	this->processed.clear();
      
	// get active airports & rwys
	for (EuroScopePlugIn::CSectorElement sfe =	this->SectorFileElementSelectFirst(EuroScopePlugIn::SECTOR_ELEMENT_RUNWAY);
												sfe.IsValid();
												sfe = this->SectorFileElementSelectNext(sfe, EuroScopePlugIn::SECTOR_ELEMENT_RUNWAY)
		)
	{
		if (sfe.IsElementActive(false, 0))
		{
			std::string aptName = vsid::utils::trim(sfe.GetAirportName());
			std::string rwyName = vsid::utils::trim(sfe.GetRunwayName(0));
			this->activeAirports[aptName].arrRwys.insert(rwyName);
		}
		if (sfe.IsElementActive(true, 0))
		{
			std::string aptName = vsid::utils::trim(sfe.GetAirportName());
			std::string rwyName = vsid::utils::trim(sfe.GetRunwayName(0));
			this->activeAirports[aptName].depRwys.insert(rwyName);
		}
		if (sfe.IsElementActive(false, 1))
		{
			std::string aptName = vsid::utils::trim(sfe.GetAirportName());
			std::string rwyName = vsid::utils::trim(sfe.GetRunwayName(1));
			this->activeAirports[aptName].arrRwys.insert(rwyName);
		}
		if (sfe.IsElementActive(true, 1))
		{
			std::string aptName = vsid::utils::trim(sfe.GetAirportName());
			std::string rwyName = vsid::utils::trim(sfe.GetRunwayName(1));
			this->activeAirports[aptName].depRwys.insert(rwyName);
		}
	}


	// only load configs if at least one airport has been selected
	if (this->activeAirports.size() > 0)
	{
		this->configParser.loadAirportConfig(this->activeAirports, this->savedRules, this->savedSettings, this->savedAreas);

		for (std::pair<const std::string, vsid::Airport> &airport : this->activeAirports)
		{
			std::set depRwys = airport.second.depRwys;
			if (airport.second.arrAsDep)
			{
				depRwys.merge(airport.second.arrRwys);
				airport.second.depRwys = depRwys;
			}
		}
		//bool aptOver = false; - see documentation below, possible usecase if sid_star should be
		//bool sidSection = false; - filtered before config and stars should be skipped

		// if there are configured airports check for remaining sid data
		for (EuroScopePlugIn::CSectorElement sfe = this->SectorFileElementSelectFirst(EuroScopePlugIn::SECTOR_ELEMENT_SIDS_STARS);
			sfe.IsValid();
			sfe = this->SectorFileElementSelectNext(sfe, EuroScopePlugIn::SECTOR_ELEMENT_SIDS_STARS)
			)
		{

			if (!this->activeAirports.count(vsid::utils::trim(sfe.GetAirportName()))) continue;

			std::string name = sfe.GetName();

			for (vsid::Sid& sid : this->activeAirports[vsid::utils::trim(sfe.GetAirportName())].sids)
			{
				if (name.substr(0, name.length() - 2) != sid.waypoint) continue;
				if (name[name.length() - 1] != sid.designator[0]) continue;

				if (std::string("0123456789").find_first_of(name[name.length() - 2]) != std::string::npos)
				{
					sid.number = name[name.length() - 2];
				}
			}
			//// DOCUMENTATION
			//if (!sidSection)
			//{
			//	if (std::string(sfe.GetAirportName()) == "EDDF")
			//	{

			//		continue;
			//	}
			//	else
			//	{
			//		sidSection = true;
			//	}
			//}
			//if (std::string(sfe.GetAirportName()) == "EDDF")
			//{
			//	messageHandler->writeMessage("DEBUG", "sfe elem: " + std::string(sfe.GetName()));
			//}

		}
	}

	// health check in case SIDs in config do not match sector file
	std::map<std::string, std::set<std::string>> incompSid;
	for (std::pair<const std::string, vsid::Airport> &aptElem : this->activeAirports)
	{
		for (vsid::Sid& sid : aptElem.second.sids)
		{
			if (std::string("0123456789").find_first_of(sid.number) == std::string::npos)
			{
				incompSid[aptElem.first].insert(sid.waypoint + '?' + sid.designator[0]);
			}
		}
	}
	// if incompatible SIDs (not in sector file) have been found remove them
	for (auto& elem : incompSid)
	{
		messageHandler->writeMessage("WARNING", "Check config for [" + elem.first + "] - Could not master sids: " + vsid::utils::join(elem.second, ','));
		for (const std::string& incompSid : elem.second)
		{
			for (auto it = this->activeAirports[elem.first].sids.begin(); it != this->activeAirports[elem.first].sids.end(); it++)
			{
				if (it->waypoint == incompSid.substr(0, incompSid.length() - 2) && it->designator[0] == incompSid[incompSid.length() - 1])
				{
					this->activeAirports[elem.first].sids.erase(it);
					break;
				}
			}
		}
	}
	messageHandler->writeMessage("INFO", "Airports updated. [" + std::to_string(this->activeAirports.size()) + "] active.");
}

void vsid::VSIDPlugin::OnTimer(int Counter)
{
	// dev test
	//vsid::Area test = { {"N050.00.49.004", "E008.31.14.804"}, {"N050.02.05.037", "E008.36.48.302"} , {"N050.03.33.122", "E008.35.56.150"} , {"N050.02.11.531", "E008.30.28.956"} };
	//test.showline();

	// end dev test
	std::pair<std::string, std::string> msg = messageHandler->getMessage();
	if (msg.first != "" && msg.second != "")
	{
		DisplayUserMessage("vSID", msg.first.c_str(), msg.second.c_str(), true, true, false, true, false);
	}

	if (this->removeProcessed.size() > 0)
	{
		auto now = std::chrono::utc_clock::now();

		for (auto it = this->removeProcessed.begin(); it != this->removeProcessed.end();)
		{
			if (now > it->second.first)
			{
				auto rm = std::erase_if(this->processed, [&](auto item) { return it->first == item.first; });
				it = this->removeProcessed.erase(it);
			}
			else ++it;
		}
	}
}
/*
* END ES FUNCTIONS
*/

void __declspec (dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn** ppPlugInInstance)
{
	//messageHandler = new vsid::MessageHandler();
	// create the instance
	*ppPlugInInstance = vsidPlugin = new vsid::VSIDPlugin();
	//*ppPlugInInstance = vsidApp.vsidPlugin = new vsid::VSIDPlugin();
}


//---EuroScopePlugInExit-----------------------------------------------

void __declspec (dllexport) EuroScopePlugInExit(void)
{
	// delete the instance
	delete vsidPlugin;
	//delete vsidApp.vsidPlugin;
}