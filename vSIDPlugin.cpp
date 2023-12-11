#include "pch.h"

#include "vSIDPlugin.h"
#include "flightplan.h"
#include "messageHandler.h"

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
	std::string route = vsid::utils::trim(FlightPlanData.GetRoute());
	std::string sidWpt;
	if (filedSid.size() > 0)
	{
		size_t len = filedSid.length();
		sidWpt = filedSid.substr(0, len - 2);
	}
	else if (route.size() > 0)
	{
		sidWpt = route.at(0);
	}
	return sidWpt;
}

vsid::sids::sid vsid::VSIDPlugin::processSid(EuroScopePlugIn::CFlightPlan FlightPlan, std::string atcRwy)
{
	EuroScopePlugIn::CFlightPlan fpln = FlightPlan;
	EuroScopePlugIn::CFlightPlanData fplnData = fpln.GetFlightPlanData();
	EuroScopePlugIn::CFlightPlanControllerAssignedData cad = fpln.GetControllerAssignedData();
	std::vector<std::string> filedRoute = vsid::utils::split(std::string(fplnData.GetRoute()), ' ');
	std::string sidWpt = vsid::VSIDPlugin::findSidWpt(fplnData);;
	vsid::sids::sid setSid;
	int prio = 99;
	bool customRuleActive = false;

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

	for(vsid::sids::sid &currSid : this->activeAirports[fplnData.GetOrigin()].sids)
	{
		bool rwyMatch = false;
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
		// if a custom rule exists 
		if (customRuleActive)
		{
			std::map<std::string, int> customRules = this->activeAirports[fplnData.GetOrigin()].customRules;
			std::vector<std::string> sidRules = vsid::utils::split(currSid.customRule, ',');

			// skip if a rule is active but it is none of the rules set in current sid
			if (!std::any_of(customRules.begin(), customRules.end(), [&sidRules](auto item)
				{
					return std::find(sidRules.begin(), sidRules.end(), item.first) != sidRules.end() && item.second;
				}
			)) continue;
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
		// skip if SID has engineNumber requirement and acft doesn't match
		if (!vsid::utils::containsDigit(currSid.engineCount, fplnData.GetEngineNumber()))
		{
			continue;
		}
		// skip if SID has WTC requirement and acft doesn't match
		if (currSid.wtc != "" && currSid.wtc.find(fplnData.GetAircraftWtc()) == std::string::npos)
		{
			continue;
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
		}
		// if a SID is accepted when filed by a pilot set the SID and break
		std::string currSidCombo = currSid.waypoint + currSid.number + currSid.designator[0];
		if (currSid.pilotfiled && currSidCombo == fplnData.GetSidName())
		{
			setSid = currSid;
			prio = currSid.prio;
		}
		if (currSid.prio < prio && setSid.pilotfiled == currSid.pilotfiled)
		{
			setSid = currSid;
			prio = currSid.prio;
		}
	}
	return(setSid);
}

void vsid::VSIDPlugin::processFlightplan(EuroScopePlugIn::CFlightPlan FlightPlan, bool checkOnly, std::string atcRwy, vsid::sids::sid manualSid)
{
	EuroScopePlugIn::CFlightPlan fpln = FlightPlan;
	EuroScopePlugIn::CFlightPlanData fplnData = fpln.GetFlightPlanData();
	EuroScopePlugIn::CFlightPlanControllerAssignedData cad = fpln.GetControllerAssignedData();
	std::string filedSidWpt = this->findSidWpt(fplnData);
	std::vector<std::string> filedRoute = vsid::utils::split(std::string(fplnData.GetRoute()), ' ');
	vsid::sids::sid sidSuggestion;
	vsid::sids::sid sidCustomSuggestion;
	//std::string sidByController;
	std::string setRwy;
	vsid::fplnInfo fplnInfo;

	vsid::fpln::clean(filedRoute, fplnData.GetOrigin(), filedSidWpt);

	/* if a sid has been set manually choose this */
	if (manualSid.waypoint != "")
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
			std::vector<std::string> rwySplit = vsid::utils::split(sidCustomSuggestion.rwy, ',');
			setRwy = rwySplit.front();
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
		ss << vsid::sids::getName(sidSuggestion) << "/" << setRwy;
		filedRoute.insert(filedRoute.begin(), vsid::utils::trim(ss.str()));
	}
	else if (sidCustomSuggestion.waypoint != "")
	{
		std::ostringstream ss;
		ss << vsid::sids::getName(sidCustomSuggestion) << "/" << setRwy;
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
	this->processed[fpln.GetCallsign()] = fplnInfo;

	if(!checkOnly)
	{	
		fplnInfo.set = false;
		if (!fplnData.SetRoute(vsid::utils::join(filedRoute).c_str()))
		{
			messageHandler->writeMessage("ERROR", "[" + std::string(fpln.GetCallsign()) + "] - Failed to change Flighplan!");
		}
		if (!fplnData.AmendFlightPlan())
		{
			messageHandler->writeMessage("ERROR", "[" + std::string(fpln.GetCallsign()) + "] - Failed to amend Flighplan!");
		}
		if (sidSuggestion.waypoint != "" && sidCustomSuggestion.waypoint == "" && sidSuggestion.initialClimb)
		{
			if (!cad.SetClearedAltitude(sidSuggestion.initialClimb))
			{
				messageHandler->writeMessage("ERROR", "Failed to set altitude.");
			}
		}
		else if (sidCustomSuggestion.waypoint != "" && sidCustomSuggestion.initialClimb)
		{
			if (!cad.SetClearedAltitude(sidCustomSuggestion.initialClimb))
			{
				messageHandler->writeMessage("ERROR", "Failed to set altitude.");
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
	/*if (!ControllerMyself().IsController())
	{
		return;
	}*/

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
		EuroScopePlugIn::CFlightPlan fpln = FlightPlanSelectASEL();
		EuroScopePlugIn::CFlightPlanData fplnData = fpln.GetFlightPlanData();
		std::string filedSidWpt = this->findSidWpt(fplnData);
		std::map<std::string, vsid::sids::sid> validDepartures;
		std::map<std::string, vsid::sids::sid> vectDepartures;
		std::string depRWY = fplnData.GetDepartureRwy();

		for (vsid::sids::sid &sid : this->activeAirports[fplnData.GetOrigin()].sids)
		{
			if (sid.waypoint == filedSidWpt && sid.rwy.find(depRWY) != std::string::npos)
			{
				validDepartures[sid.waypoint + sid.number + sid.designator[0]] = sid;
				validDepartures[sid.waypoint + 'R' + 'V'] = {sid.waypoint, 'R', "V", depRWY};
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
		}
		
		if (strlen(sItemString) != 0 && std::string(sItemString) != "NO SID")
		{
			this->processFlightplan(fpln, false, depRWY, validDepartures[sItemString]);
		}
		// FlightPlan->flightPlan.GetControllerAssignedData().SetFlightStripAnnotation(0, sItemString) // test on how to set annotations (-> exclusive per plugin)
	}

	if (FunctionId == TAG_FUNC_VSID_SIDS_AUTO)
	{
		EuroScopePlugIn::CFlightPlan fpln = FlightPlanSelectASEL();
		if (this->processed.find(fpln.GetCallsign()) != this->processed.end() && fpln.IsValid())
		{
			EuroScopePlugIn::CFlightPlanData fplnData = fpln.GetFlightPlanData();
			std::vector<std::string> filedRoute = vsid::utils::split(fplnData.GetRoute(), ' ');
			std::string atcRwy = vsid::fpln::getAtcBlock(filedRoute, fplnData.GetOrigin()).second;
			this->processFlightplan(FlightPlanSelectASEL(), false, atcRwy);            //////////////// possible unhandled exception: VFR flight or flight without runway
		}
		else
		{
			this->processFlightplan(FlightPlanSelectASEL(), false);
		}
	}

	if (FunctionId == TAG_FUNC_VSID_CLMBMENU)
	{
		EuroScopePlugIn::CFlightPlan fpln = FlightPlanSelectASEL();
		EuroScopePlugIn::CFlightPlanData fplnData = fpln.GetFlightPlanData();
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
				messageHandler->writeMessage("ERROR", "Failed to set cleared altitude");
			}
		}
	}

	if (FunctionId == TAG_FUNC_VSID_RWYMENU)
	{
		EuroScopePlugIn::CFlightPlan fpln = FlightPlanSelectASEL();
		EuroScopePlugIn::CFlightPlanData fplnData = fpln.GetFlightPlanData();
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
				messageHandler->writeMessage("ERROR", "[" + std::string(fpln.GetCallsign()) + "] - Failed to change Flighplan!");
			}
			if (!fplnData.AmendFlightPlan())
			{
				messageHandler->writeMessage("ERROR", "[" + std::string(fpln.GetCallsign()) + "] - Failed to amend Flighplan!");
			}
		}
	}
}

void vsid::VSIDPlugin::OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize)
{
	// OUTSTANDING TEST: IS ALU PART OF ES GROUNDSTATES?
	/*std::string iscleared = (FlightPlan.GetClearenceFlag()) ? "is cleared" : "is not cleared";
	if (std::string(FlightPlan.GetGroundState()) != "")
	{
		messageHandler->writeMessage("DEBUG", "[" + std::string(FlightPlan.GetCallsign()) + "]: " + FlightPlan.GetGroundState() + " " + iscleared);
	}*/
	
	if (ItemCode == TAG_ITEM_VSID_SIDS)
	{
		*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
		std::string callsign = FlightPlan.GetCallsign();

		if (!FlightPlan.IsValid())
		{
			messageHandler->writeMessage("WARNING", "[" + callsign + "] flightplan is invalid according to ES.");
			return;
		}

		if (this->processed.find(callsign) != this->processed.end())
		{
			/*if (!FlightPlan.GetClearenceFlag())
			{
				EuroScopePlugIn::CFlightPlanData fplnData = FlightPlan.GetFlightPlanData();
				std::vector<std::string> filedRoute = vsid::utils::split(fplnData.GetRoute(), ' ');
				std::pair<std::string, std::string> atcBlock = vsid::fpln::clean(filedRoute, fplnData.GetOrigin());
				if (atcBlock.second != "" &&
					this->activeAirports[fplnData.GetOrigin()].depRwys.find(atcBlock.second) !=
					this->activeAirports[fplnData.GetOrigin()].depRwys.end())
				{
					this->processed.erase(FlightPlan.GetCallsign());
					this->processFlightplan(FlightPlan, true);
				}
			}*/
			EuroScopePlugIn::CFlightPlanData fplnData = FlightPlan.GetFlightPlanData();
			std::pair<std::string, std::string> atcBlock = vsid::fpln::getAtcBlock(vsid::utils::split(fplnData.GetRoute(), ' '), fplnData.GetOrigin());
			std::string sidName = vsid::sids::getName(this->processed[callsign].sid);
			std::string customSidName = vsid::sids::getName(this->processed[callsign].customSid);

			if (atcBlock.first == fplnData.GetOrigin() && vsid::sids::isEmpty(this->processed[callsign].customSid))
			{
				*pRGB = this->configParser.getColor("noSid");
			}
			else if (atcBlock.first == "" ||
					atcBlock.first == fplnData.GetOrigin() &&
					(vsid::sids::isEmpty(this->processed[callsign].customSid) ||
					this->processed[callsign].sid == this->processed[callsign].customSid)
					)
			{
				*pRGB = this->configParser.getColor("sidSuggestion");
			}
			else if (atcBlock.first != "" &&
					atcBlock.first == fplnData.GetOrigin() &&
					!vsid::sids::isEmpty(this->processed[callsign].customSid) &&
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
			else if (atcBlock.first != "" && atcBlock.first == fplnData.GetOrigin() && this->processed[callsign].customSid.rwy.find(atcBlock.second) == std::string::npos)
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
		else if(std::string(FlightPlan.GetFlightPlanData().GetPlanType()) == "I")
		{
			this->processFlightplan(FlightPlan, true);
		}
	}

	if (ItemCode == TAG_ITEM_VSID_CLIMB)
	{
		*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
		EuroScopePlugIn::CFlightPlan fpln = FlightPlan;
		EuroScopePlugIn::CFlightPlanData fplnData = fpln.GetFlightPlanData();
		std::string callsign = FlightPlan.GetCallsign();
		int transAlt = 0;
		int maxInitialClimb = 0;

		if (this->processed.find(callsign) != this->processed.end())
		{
			// check to be able to determine if a found non-standard sid exists in valid sids and set it for ic comparison
			std::string atcSid = vsid::fpln::getAtcBlock(vsid::utils::split(fplnData.GetRoute(), ' '), fplnData.GetOrigin()).first;
			if (atcSid != "" &&
				atcSid != fplnData.GetOrigin() &&
				atcSid != vsid::sids::getName(this->processed[callsign].sid) &&
				vsid::sids::isEmpty(this->processed[callsign].customSid))
			{
				for (const vsid::sids::sid& sid : this->activeAirports[fplnData.GetOrigin()].sids)
				{
					if (sid.waypoint != atcSid.substr(0, atcSid.length() - 2)) continue;
					if (sid.designator[0] != atcSid[atcSid.length() - 1]) continue;
					this->processed[callsign].customSid = sid;
					break;
				}
			}
			if (fpln.GetClearedAltitude() == fpln.GetFinalAltitude())
			{
				*pRGB = this->configParser.getColor("suggestedClmb"); // white
			}
			else if (fpln.GetClearedAltitude() != fpln.GetFinalAltitude() &&
					(fpln.GetClearedAltitude() == this->processed[callsign].sid.initialClimb &&
					vsid::fpln::getAtcBlock(vsid::utils::split(fplnData.GetRoute(), ' '), fplnData.GetOrigin()).first ==
					vsid::sids::getName(this->processed[callsign].sid)) ||
					(fpln.GetClearedAltitude() == this->processed[callsign].customSid.initialClimb &&
					vsid::fpln::getAtcBlock(vsid::utils::split(fplnData.GetRoute(), ' '), fplnData.GetOrigin()).first ==
					vsid::sids::getName(this->processed[callsign].customSid))
					)
			{
				if (this->processed[callsign].sid.climbvia)
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

			transAlt = this->activeAirports[fplnData.GetOrigin()].transAlt;
			maxInitialClimb = this->activeAirports[fplnData.GetOrigin()].maxInitialClimb;
			if (fpln.GetClearedAltitude() == fpln.GetFinalAltitude() && std::string(fplnData.GetPlanType()) != "V")
			{
				if (this->processed[callsign].sid.initialClimb == 0)
				{
					strcpy_s(sItemString, 16, std::string("---").c_str());
				}
				else if (this->processed[callsign].sid.initialClimb <= transAlt)
				{
					strcpy_s(sItemString, 16, std::string("A").append(std::to_string(this->processed[callsign].sid.initialClimb / 100)).c_str());
				}
				else
				{
					if (this->processed[callsign].sid.initialClimb / 100 >= 100)
					{
						strcpy_s(sItemString, 16, std::to_string(this->processed[callsign].sid.initialClimb / 100).c_str());
					}
					else
					{
						strcpy_s(sItemString, 16, std::string("0").append(std::to_string(this->processed[callsign].sid.initialClimb / 100)).c_str());
					}
				}
			}
			else
			{
				if (fpln.GetClearedAltitude() == 0 ||
					(this->processed[callsign].sid.initialClimb == 0 && std::string(fplnData.GetPlanType()) != "V")
					)
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
		EuroScopePlugIn::CFlightPlanData fplnData = FlightPlan.GetFlightPlanData();

		std::vector<std::string> filedRoute = vsid::utils::split(fplnData.GetRoute(), ' ');
		std::pair<std::string, std::string> atcBlock = vsid::fpln::getAtcBlock(filedRoute, fplnData.GetOrigin());
		
		// rwy set together with SID
		if ((this->processed.find(FlightPlan.GetCallsign()) != this->processed.end() &&
			atcBlock.second != "" &&
			this->activeAirports[fplnData.GetOrigin()].depRwys.find(atcBlock.second) !=
			this->activeAirports[fplnData.GetOrigin()].depRwys.end())/* ||
			fplnData.IsAmended()*/
			)
		{
			*pRGB = this->configParser.getColor("rwySet");
		}
		else if(this->processed.find(FlightPlan.GetCallsign()) != this->processed.end() &&
				atcBlock.second != "" &&
				this->activeAirports[fplnData.GetOrigin()].depRwys.find(atcBlock.second) ==
				this->activeAirports[fplnData.GetOrigin()].depRwys.end()/* && - DISABLED UNTIL ANSWER FROM ES DEV
				fplnData.IsAmended()*/
				)
		{
			*pRGB = this->configParser.getColor("notDepRwySet");
		}
		else *pRGB = this->configParser.getColor("rwyNotSet");

		if (this->processed.find(FlightPlan.GetCallsign()) != this->processed.end())
		{
			if (atcBlock.second != "" && fplnData.IsAmended())
			{
				strcpy_s(sItemString, 16, atcBlock.second.c_str());
			}
			else
			{
				std::string sidRwy;
				if (this->processed[FlightPlan.GetCallsign()].sid.rwy.find(',') != std::string::npos)
				{
					sidRwy = vsid::utils::split(this->processed[FlightPlan.GetCallsign()].sid.rwy, ',').front();
				}
				else
				{
					sidRwy = this->processed[FlightPlan.GetCallsign()].sid.rwy;
				}
				strcpy_s(sItemString, 16, sidRwy.c_str());
			}
		}
		else strcpy_s(sItemString, 16, fplnData.GetDepartureRwy());
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
				"rule [icao] [rulename] - toggle rule for icao / "
				"lvp [icao] - toggle lvp ops for icao / "
				"Debug - toggle debug mode (inop)");
			return true;
		}
		if (vsid::utils::tolower(command[1]) == "version")
		{
			messageHandler->writeMessage("INFO", "vSID Version " + pluginVersion + " loaded.");
			return true;
		}
		else if (vsid::utils::tolower(command[1]) == "rule")
		{
			if (command.size() == 2)
			{
				for (std::pair<const std::string, vsid::airport>& airport : this->activeAirports)
				{
					if (!airport.second.customRules.empty())
					{
						std::ostringstream ss;
						for (std::pair<const std::string, int>& rule : airport.second.customRules)
						{
							std::string status = (rule.second) ? "ON" : "OFF";
							ss << rule.first << ": " << status << " ";
						}
						messageHandler->writeMessage(airport.first + " Rules", ss.str());
					}
					else messageHandler->writeMessage(airport.first + " Rules", "No rules configured");
				}
			}
			else if (command.size() >= 3 && this->activeAirports.count(vsid::utils::toupper(command[2])))
			{
				std::string icao = vsid::utils::toupper(command[2]);
				if (command.size() == 3)
				{
					if (!this->activeAirports[icao].customRules.empty())
					{
						std::ostringstream ss;
						for (std::pair<const std::string, int>& rule : this->activeAirports[icao].customRules)
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
					if (this->activeAirports[icao].customRules.find(rule) != this->activeAirports[icao].customRules.end())
					{
						if (this->activeAirports[icao].customRules[rule])
						{
							this->activeAirports[icao].customRules[rule] = 0;
						}
						else
						{
							this->activeAirports[icao].customRules[rule] = 1;
						}
						std::string status = (this->activeAirports[icao].customRules[rule]) ? "ON" : "OFF";
						messageHandler->writeMessage(icao + " Rule", command[3] + " " + status);
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
					messageHandler->writeMessage("INFO", "Updating airports...");
					this->UpdateActiveAirports();
				}
				else messageHandler->writeMessage("INFO", vsid::utils::toupper(command[2]) + " is not in active airports");
			}
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
	if (!FlightPlan.IsValid())
	{
		std::string callsign = FlightPlan.GetCallsign();
		messageHandler->writeMessage("WARNING", "[" + callsign + "] flightplan is invalid according to ES.");
		return;
	}
	if (this->processed.find(FlightPlan.GetCallsign()) != this->processed.end())
	{
		EuroScopePlugIn::CFlightPlanData fplnData = FlightPlan.GetFlightPlanData();
		std::vector<std::string> filedRoute = vsid::utils::split(fplnData.GetRoute(), ' ');
		if (filedRoute.size() > 0)
		{
			std::pair<std::string, std::string> atcBlock = vsid::fpln::getAtcBlock(filedRoute, fplnData.GetOrigin());
			if (atcBlock.first == FlightPlan.GetFlightPlanData().GetOrigin() && atcBlock.second != "")
			{
				this->processFlightplan(FlightPlan, true, atcBlock.second);
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
	this->processed.erase(FlightPlan.GetCallsign());
}

void vsid::VSIDPlugin::OnControllerDisconnect(EuroScopePlugIn::CController Controller) // use this to check for lower atc disconnects to start automation again
{
	//vsid::messagehandler::LogMessage("DEBUG", "Controller disconnected: " + std::string(Controller.GetFullName()));
	if (Controller.GetPositionId() == ControllerMyself().GetPositionId())
	{
		//vsid::messagehandler::LogMessage("DEBUG", "I disconnected!");
	}
}

void vsid::VSIDPlugin::OnAirportRunwayActivityChanged()
{
	this->UpdateActiveAirports();
}

void vsid::VSIDPlugin::UpdateActiveAirports()
{
	this->savedSettings.clear();
	for (std::pair<const std::string, vsid::airport>& airport : this->activeAirports)
	{
		this->savedSettings.insert({ airport.first, airport.second.settings });
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
		this->configParser.loadAirportConfig(this->activeAirports, this->savedSettings);

		for (std::pair<const std::string, vsid::airport> &airport : this->activeAirports)
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

			for (vsid::sids::sid& sid : this->activeAirports[vsid::utils::trim(sfe.GetAirportName())].sids)
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
	for (std::pair<const std::string, vsid::airport> &aptElem : this->activeAirports)
	{
		for (vsid::sids::sid& sid : aptElem.second.sids)
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
}

void vsid::VSIDPlugin::OnTimer(int Counter)
{
	std::pair<std::string, std::string> msg = messageHandler->getMessage();
	if (msg.first != "" && msg.second != "")
	{
		DisplayUserMessage("vSID", msg.first.c_str(), msg.second.c_str(), true, true, false, true, false);
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