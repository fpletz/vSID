#include "pch.h"

#include "vSIDPlugin.h"
#include "messageHandler.h"

#include <set>

vsid::VSIDPlugin* vsidPlugin;

vsid::VSIDPlugin::VSIDPlugin() : EuroScopePlugIn::CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, pluginName.c_str(), pluginVersion.c_str(), pluginAuthor.c_str(), pluginCopyright.c_str()) {

	this->configParser.loadMainConfig();
	this->configParser.loadGrpConfig();
	
	RegisterTagItemType("vSID SID", TAG_ITEM_VSID_SIDS);
	RegisterTagItemFunction("SIDs Auto Select", TAG_FUNC_VSID_SIDS_AUTO);
	RegisterTagItemFunction("SIDs Menu", TAG_FUNC_VSID_SIDS_MAN);

	RegisterTagItemType("vSID CLMB", TAG_ITEM_VSID_CLIMB);
	RegisterTagItemFunction("Climb Menu", TAG_FUNC_VSID_CLMBMENU);

	UpdateActiveAirports(); // preload rwy settings

	DisplayUserMessage("Message", "vSID", std::string("Version " + pluginVersion + " loaded").c_str(), true, true, false, false, false);
}

vsid::VSIDPlugin::~VSIDPlugin() {}

/*
* BEGIN OWN FUNCTIONS
*/

bool vsid::VSIDPlugin::getDebug()
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

		// TODO: test for functionality. should break outer loop if SID has been set (=^ pilot filed sid or 2 engine heavy)
		/*if (setSid.waypoint != "")
		{
			break;
		}*/
	for(vsid::sids::sid &currSid : this->activeAirports[fplnData.GetOrigin()].sids)
	{
		bool rwyMatch = false;
		// skip if current SID does not match found SID wpt
		if (currSid.waypoint != sidWpt)
		{	
			continue;
		}
		// check for rwy match, only if match found "return" true - otherwise sid would be selected due to missing skip for outer loop
		for (std::string depRwy : this->activeAirports[fplnData.GetOrigin()].depRwys)
		{
			// skip if a rwy has been set manually and it doesn't match available sid rwys
			if (atcRwy != "" && atcRwy != depRwy)
			{
				continue;
			}
			/*else if (atcRwy != "" && atcRwy == depRwy)
			{
				rwyMatch = true;
				break;
			}*/
			// skip if airport dep rwys are not part of the SID
			if (~currSid.rwy.find(depRwy) != std::string::npos)
			{
				continue;
			}
			else
			{
				rwyMatch = true;
				//break;
			}
		}
		// skip if no rwy was found in SID;
		if (!rwyMatch) continue;
		// skip if engine type doesn't match
		if (currSid.engineType != "" && ~currSid.engineType.find(fplnData.GetEngineType()) != std::string::npos)
		{
			continue;
		}
		// skip if SID has engineNumber requirement and acft doesn't match
		if (!vsid::utils::containsDigit(currSid.engineCount, fplnData.GetEngineNumber()))
		{
			continue;
		}
		// skip if SID has WTC requirement and acft doesn't match
		if (currSid.wtc != "" && ~currSid.wtc.find(fplnData.GetAircraftWtc()) != std::string::npos)
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
			for (auto it = this->configParser.grpConfig.begin(); it != this->configParser.grpConfig.end(); ++it)
			{
				if (it->value("ICAO", "") != acftType) continue;
				if (it->value("MTOW", 0) > currSid.mtow) break; // acft icao found but to heavy, no further checks
				else break; // acft light enough, no further checks
			}
		}
		// if a SID is accepted when filed by a pilot set the SID and break
		std::string currSidCombo = currSid.waypoint + currSid.number + currSid.designator;
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
	std::vector<std::string> filedRoute = vsid::utils::split(std::string(fplnData.GetRoute()), ' ');
	std::string sidWpt = vsid::VSIDPlugin::findSidWpt(fplnData);
	vsid::sids::sid sidSuggestionRaw;
	vsid::sids::sid sidCustomSuggestionRaw;
	std::string sidByController;
	std::string rwyByAtc;
	std::string setRwy;
	vsid::fplnInfo fplnInfo;

	/* if a sid has been set manually choose this */
	if (manualSid.waypoint != "")
	{
		sidSuggestionRaw = manualSid;
		sidSuggestionRaw.rwy = atcRwy; // force dep rwy
	}
	/* if a rwy is given by atc check for a sid for this rwy and for a normal sid
	* to be then able to compare those two
	*/
	else if (atcRwy != "")
	{
		sidSuggestionRaw = this->processSid(fpln, "");
		sidCustomSuggestionRaw = this->processSid(fpln, atcRwy);
		if (sidCustomSuggestionRaw.waypoint == "")
		{
			sidCustomSuggestionRaw.waypoint = "manual";
		}
	}
	/* default state */
	else
	{
		sidSuggestionRaw = this->processSid(fpln, atcRwy);
	}

	/* combined SID if one has been found */
	std::string sidSuggestion;
	if (sidSuggestionRaw.waypoint != "")
	{
		sidSuggestion = sidSuggestionRaw.waypoint + sidSuggestionRaw.number + sidSuggestionRaw.designator;
	}
	/* combined 'custom' SID if one has been found */
	std::string sidCustomSuggestion;
	if (sidCustomSuggestionRaw.waypoint != "" && sidCustomSuggestionRaw.waypoint != "manual")
	{
		sidCustomSuggestion = sidCustomSuggestionRaw.waypoint + sidCustomSuggestionRaw.number + sidCustomSuggestionRaw.designator;
	}

	//
	// possible improvement on .erase() with std::remove
	//
	/* Strip the filed route from SID/RWY and/or SID to have a bare route to populate with set SID */
	if (filedRoute.size() > 0)
	{
		if (filedRoute.front().find('/') != std::string::npos && !(filedRoute.front().find("/N") != std::string::npos))
		{
			std::vector<std::string> sidBlock = vsid::utils::split(filedRoute.at(0), '/');
			if (sidBlock.front().find_first_of("0123456789RV") != std::string::npos && sidBlock.front() != fplnData.GetOrigin())
			{
				sidByController = sidBlock.front();
			}
			if (sidBlock.front() == fplnData.GetOrigin())
			{
				rwyByAtc = sidBlock.back();
			}
			filedRoute.erase(filedRoute.begin());
		}
	}
	/* if a possible SID block was found check the entire route for more SIDs (e.g. filed) and erase them as well*/
	if (filedRoute.size() > 0)
	{
		for (std::vector<std::string>::iterator it = filedRoute.begin(); it != filedRoute.end();)
		{
			if (sidByController != "")
			{
				if (it->substr(0, it->length() - 2) == sidByController.substr(0, sidByController.length() - 2))
				{
					it = filedRoute.erase(it);
				}
				else
				{
					++it;
				}
			}
			else
			{
				if (it->substr(0, it->length() - 2) == sidWpt)
				{
					it = filedRoute.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
	}

	if (sidSuggestion != "" && sidCustomSuggestion == "")
	{
		if (sidSuggestionRaw.rwy.find(',') != std::string::npos)
		{
			std::vector<std::string> rwySplit = vsid::utils::split(sidSuggestionRaw.rwy, ',');
			for (std::string& rwy : rwySplit)
			{
				if (this->activeAirports[fplnData.GetOrigin()].depRwys.count(rwy))
				{
					setRwy = rwy;
				}
			}
			if (setRwy == "")
			{
				setRwy = rwySplit.front();
			}
		}
		else
		{
			setRwy = sidSuggestionRaw.rwy;
		}
	}
	else if (sidCustomSuggestion != "" && sidCustomSuggestionRaw.waypoint != "manual")
	{
		if (sidCustomSuggestionRaw.rwy.find(',') != std::string::npos)
		{
			std::vector<std::string> rwySplit = vsid::utils::split(sidCustomSuggestionRaw.rwy, ',');
			setRwy = rwySplit.front();
		}
		else
		{
			setRwy = sidCustomSuggestionRaw.rwy;
		}
	}
	

	// building a new route with the selected sid
	if (sidSuggestionRaw.waypoint != "" && (sidCustomSuggestionRaw.waypoint == "" || sidCustomSuggestionRaw.waypoint == "manual"))
	{
		std::ostringstream ss;
		ss << sidSuggestionRaw.waypoint + sidSuggestionRaw.number + sidSuggestionRaw.designator << "/" << setRwy;
		filedRoute.insert(filedRoute.begin(), vsid::utils::trim(ss.str()));
	}
	if (sidCustomSuggestionRaw.waypoint != "" && sidCustomSuggestionRaw.waypoint != "manual")
	{
		std::ostringstream ss;
		ss << sidCustomSuggestionRaw.waypoint + sidCustomSuggestionRaw.number + sidCustomSuggestionRaw.designator << "/" << setRwy;
		filedRoute.insert(filedRoute.begin(), vsid::utils::trim(ss.str()));
	}

	if (checkOnly)
	{
		if (rwyByAtc != "")
		{
			fplnInfo.atcRwy = rwyByAtc;
		}
		// sid set in fpln by local atc but does not match plugin suggestion
		if (sidByController == "" && sidCustomSuggestion != "" && sidCustomSuggestion != sidSuggestion)
		{
			fplnInfo.set = true;
			fplnInfo.sid = sidCustomSuggestion;
			fplnInfo.sidColor = this->configParser.getColor("customSidSuggestion"); // yellow
			fplnInfo.clmb = sidCustomSuggestionRaw.initialClimb;
			fplnInfo.clmbVia = sidCustomSuggestionRaw.climbvia;
		}
		// sid set in fpln by remote atc but does not match plugin suggestion
		else if (sidByController != "" && sidByController != sidSuggestion)
		{
			if (this->processed[FlightPlan.GetCallsign()].set)
			{
				fplnInfo.set = false;
			}
			fplnInfo.sid = sidByController;
			fplnInfo.sidColor = this->configParser.getColor("customSidSet"); // orange
			//for (vsid::airport& apt : this->activeAirports) // possible performance improvement if needed - check only if fplnInfo.clmb = req. final level else get set val. in .clmb

			for (vsid::sids::sid& sid : this->activeAirports[fplnData.GetOrigin()].sids)
			{
				if (sid.waypoint != sidByController.substr(0, sidByController.length() - 2))
				{
					continue;
				}
				if (sid.number != sidByController.at(sidByController.length() - 2))
				{
					continue;
				}
				if (sid.designator != sidByController.at(sidByController.length() - 1))
				{
					continue;
				}
				fplnInfo.clmb = sid.initialClimb;
				fplnInfo.clmbVia = sid.climbvia;
				break;
			}
		}
		// sid set in fpln by any atc and matches plugin suggestion
		else if (sidByController != "" && sidSuggestion == sidByController)
		{
			if (this->processed[FlightPlan.GetCallsign()].set)
			{
				fplnInfo.set = true;
			}
			else
			{
				fplnInfo.set = false;
			}
			fplnInfo.sid = sidByController;
			fplnInfo.sidColor = this->configParser.getColor("suggestedSidSet"); // green
			fplnInfo.clmb = sidSuggestionRaw.initialClimb;
			fplnInfo.clmbVia = sidSuggestionRaw.climbvia;
		}
		// set the plugin sid to white (not set in fpln yet)
		else if (sidByController == "")
		{
			fplnInfo.set = false;
			fplnInfo.sid = sidSuggestion;
			fplnInfo.sidColor = this->configParser.getColor("sidSuggestion"); // white
			fplnInfo.clmb = sidSuggestionRaw.initialClimb;
			fplnInfo.clmbVia = sidSuggestionRaw.climbvia;
		}
	}
	else
	{	
		// DEACTIVATED DURING DEVELOPMENT
		fplnInfo.set = false;
		if (!fplnData.SetRoute(vsid::utils::join(filedRoute).c_str()))
		{
			messageHandler->writeMessage("ERROR", "[" + std::string(fpln.GetCallsign()) + "] - Failed to change Flighplan!");
		}
		else
		{
			if (sidSuggestion != "" && sidCustomSuggestion == "")
			{
				fplnInfo.set = true;
				fplnInfo.sid = sidSuggestion;
				if (sidSuggestion.find_first_of("RV") != std::string::npos)
				{
					fplnInfo.sidColor = this->configParser.getColor("customSidSet"); // orange
				}
				else
				{
					fplnInfo.sidColor = this->configParser.getColor("suggestedSidSet"); // green
				}
				fplnInfo.clmb = sidSuggestionRaw.initialClimb;
				fplnInfo.clmbVia = sidSuggestionRaw.climbvia;
			}
			else if (sidCustomSuggestion != "")
			{
				fplnInfo.set = true;
				fplnInfo.sid = sidCustomSuggestion;
				fplnInfo.sidColor = this->configParser.getColor("customSidSet"); // orange
				fplnInfo.clmb = sidCustomSuggestionRaw.initialClimb;
				fplnInfo.clmbVia = sidCustomSuggestionRaw.climbvia;

			}
			/*else if (sidSuggestion != "" && (sidCustomSuggestion.find_first_of("RV") != std::string::npos))
			{
				fplnInfo.set = true;
				fplnInfo.tagString = sidSuggestion;
				fplnInfo.tagColor = this->configParser.getColor("customSidSet");
			}*/
		}
		if (!fplnData.AmendFlightPlan())
		{
			messageHandler->writeMessage("ERROR", "[" + std::string(fpln.GetCallsign()) + "] - Failed to amend Flighplan!");
		}
		if (sidSuggestion != "" && sidCustomSuggestion == "" && sidSuggestionRaw.initialClimb)
		{
			if (!cad.SetClearedAltitude(sidSuggestionRaw.initialClimb))
			{
				messageHandler->writeMessage("ERROR", "Failed to set altitude.");
			}
		}
		else if (sidCustomSuggestion != "" && sidCustomSuggestionRaw.initialClimb)
		{
			if (!cad.SetClearedAltitude(sidCustomSuggestionRaw.initialClimb))
			{
				messageHandler->writeMessage("ERROR", "Failed to set altitude.");
			}
		}
	}
	if (sidSuggestionRaw.waypoint == "" && sidByController == "")
	{
		fplnInfo.sid = "NO SID";
		fplnInfo.sidColor = this->configParser.getColor("noSid"); // red
		fplnInfo.clmb = 0;
	}
	if (sidCustomSuggestionRaw.waypoint == "manual")
	{
		fplnInfo.sid = "MANUAL";
		fplnInfo.sidColor = this->configParser.getColor("noSid"); // red
		fplnInfo.clmb = 0;
	}
	this->processed[fpln.GetCallsign()] = fplnInfo;
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
		std::string filedSidWpt = vsid::VSIDPlugin::findSidWpt(fplnData);
		std::map<std::string, vsid::sids::sid> validDepartures;
		std::map<std::string, vsid::sids::sid> vectDepartures;
		std::string depRWY = fplnData.GetDepartureRwy();

		for (vsid::sids::sid &sid : this->activeAirports[fplnData.GetOrigin()].sids)
		{
			if (sid.waypoint == filedSidWpt && sid.rwy.find(depRWY) != std::string::npos)
			{
				validDepartures[sid.waypoint + sid.number + sid.designator] = sid;
				validDepartures[sid.waypoint + 'R' + 'V'] = { sid.waypoint, 'R', 'V', depRWY };
			}
		}

		this->OpenPopupList(Area, "Select SID", 1);
		if (validDepartures.size() == 0)
		{
			this->AddPopupListElement("NO SID", "NO SID", TAG_FUNC_VSID_SIDS_MAN, false, EuroScopePlugIn::POPUP_ELEMENT_NO_CHECKBOX, true, false); // possibly set disabled to false as NO SID is set when rwy closed but avbl sid for rwy selected
		}
		for (const auto &sid : validDepartures)
		{
			this->AddPopupListElement(sid.first.c_str(), sid.first.c_str(), TAG_FUNC_VSID_SIDS_MAN, false, EuroScopePlugIn::POPUP_ELEMENT_NO_CHECKBOX, false, false);
		}
		if (strlen(sItemString) != 0)
		{
			if (std::string(sItemString, strlen(sItemString)-2, strlen(sItemString)) != "Vectors")
			{
				this->processFlightplan(fpln, false, depRWY, validDepartures[sItemString]);
			}
		}
		// FlightPlan->flightPlan.GetControllerAssignedData().SetFlightStripAnnotation(0, sItemString) // test on how to set annotations (-> exclusive per plugin)
	}

	if (FunctionId == TAG_FUNC_VSID_SIDS_AUTO)
	{
		EuroScopePlugIn::CFlightPlan fpln = FlightPlanSelectASEL();
		if (this->processed.find(fpln.GetCallsign()) != this->processed.end() && fpln.IsValid())
		{
			this->processFlightplan(FlightPlanSelectASEL(), false, this->processed[fpln.GetCallsign()].atcRwy);
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
}

void vsid::VSIDPlugin::OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize)
{
	if (ItemCode == TAG_ITEM_VSID_SIDS)
	{
		*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
		std::string callsign = FlightPlan.GetCallsign();

		if (this->processed.find(callsign) != this->processed.end())
		{
			*pRGB = this->processed[callsign].sidColor;
			strcpy_s(sItemString, 16, this->processed[callsign].sid.c_str());
		}
		else
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
			if (fpln.GetClearedAltitude() == fpln.GetFinalAltitude())
			{
				*pRGB = this->configParser.getColor("suggestedClmb"); // white
			}
			else if ((fpln.GetClearedAltitude() != fpln.GetFinalAltitude()) && (fpln.GetClearedAltitude() == this->processed[callsign].clmb))
			{
				if (this->processed[callsign].clmbVia)
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
			if (fpln.GetClearedAltitude() == fpln.GetFinalAltitude())
			{
				if (this->processed[callsign].clmb == 0)
				{
					strcpy_s(sItemString, 16, std::string("---").c_str());
				}
				else if (this->processed[callsign].clmb <= transAlt)
				{
					strcpy_s(sItemString, 16, std::string("A").append(std::to_string(this->processed[callsign].clmb / 100)).c_str());
				}
				else
				{
					if (this->processed[callsign].clmb / 100 >= 100)
					{
						strcpy_s(sItemString, 16, std::to_string(this->processed[callsign].clmb / 100).c_str());
					}
					else
					{
						strcpy_s(sItemString, 16, std::string("0").append(std::to_string(this->processed[callsign].clmb / 100)).c_str());
					}
				}
			}
			else
			{
				if (fpln.GetClearedAltitude() == 0 || this->processed[callsign].clmb == 0)
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
}

bool vsid::VSIDPlugin::OnCompileCommand(const char* sCommandLine)
{
	std::string commandLine = sCommandLine;

	if (commandLine == ".vsid debug")
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
	return false;
}


void vsid::VSIDPlugin::OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan)
{
	// if we receive updates for flightplans validate entered sid again to sync between controllers
	// updates are received for any flightplan not just that under control

	if (this->processed.find(FlightPlan.GetCallsign()) != this->processed.end())
	{
		EuroScopePlugIn::CFlightPlanData fplnData = FlightPlan.GetFlightPlanData();
		std::vector<std::string> filedRoute = vsid::utils::split(fplnData.GetRoute(), ' ');
		if (filedRoute.size() > 0)
		{
			if (filedRoute.at(0).find('/') != std::string::npos)
			{
				std::string setOrigin = vsid::utils::split(filedRoute.at(0), '/').front();
				if (setOrigin == fplnData.GetOrigin())
				{
					this->processFlightplan(FlightPlan, true, vsid::utils::split(filedRoute.at(0), '/').back());
				}
				else
				{
					this->processFlightplan(FlightPlan, true);
				}
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
		this->configParser.loadAirportConfig(this->activeAirports);

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
				if (name[name.length() - 1] != sid.designator) continue;

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
				incompSid[aptElem.first].insert(sid.waypoint + '?' + sid.designator);
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
				if (it->waypoint == incompSid.substr(0, incompSid.length() - 2) && it->designator == incompSid[incompSid.length() - 1])
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