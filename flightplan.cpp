#include "pch.h"

#include "flightplan.h"
#include "utils.h"

// dev
#include "messageHandler.h"
// end dev

void vsid::fpln::clean(std::vector<std::string> &filedRoute, const std::string origin, std::string filedSidWpt)
{
	std::pair<std::string, std::string> atcBlock = getAtcBlock(filedRoute, origin);

	if (filedRoute.size() > 0)
	{
		if (filedRoute.front().find('/') != std::string::npos && filedRoute.front().find("/N") == std::string::npos)
		{
			filedRoute.erase(filedRoute.begin());
		}
	}
	/* if a possible SID block was found check the entire route until the sid waypoint is found*/
	if (filedRoute.size() > 0 && filedSidWpt != "")
	{
		for (std::vector<std::string>::iterator it = filedRoute.begin(); it != filedRoute.end();)
		{
			if (*it == filedSidWpt) break;
			it = filedRoute.erase(it);
		}
	}
}

std::pair<std::string, std::string> vsid::fpln::getAtcBlock(const std::vector<std::string>& filedRoute, const std::string origin)
{
	std::string atcRwy = "";
	std::string atcSid = "";
	if (filedRoute.size() > 0)
	{
		if (filedRoute.front().find('/') != std::string::npos && filedRoute.front().find("/N") == std::string::npos)
		{
			std::vector<std::string> sidBlock = vsid::utils::split(filedRoute.at(0), '/');
			if (sidBlock.front().find_first_of("0123456789RV") != std::string::npos || sidBlock.front() == origin)
			{
				atcSid = sidBlock.front();
				atcRwy = sidBlock.back();
			}
		}
	}
	return { atcSid, atcRwy };
}

bool vsid::fpln::findRemarks(const EuroScopePlugIn::CFlightPlanData& fplnData, const std::string(& searchStr))
{
	return std::string(fplnData.GetRemarks()).find(searchStr) != std::string::npos;
}

bool vsid::fpln::removeRemark(EuroScopePlugIn::CFlightPlanData& fplnData, const std::string(&toRemove))
{
	std::vector<std::string> remarks = vsid::utils::split(fplnData.GetRemarks(), ' ');
	for (std::vector<std::string>::iterator it = remarks.begin(); it != remarks.end();)
	{
		if (*it == toRemove)
		{
			it = remarks.erase(it);
		}
		else if (it != remarks.end()) ++it;
	}
	return fplnData.SetRemarks(vsid::utils::join(remarks).c_str());
}

bool vsid::fpln::addRemark(EuroScopePlugIn::CFlightPlanData& fplnData, const std::string(&toAdd))
{
	std::vector<std::string> remarks = vsid::utils::split(fplnData.GetRemarks(), ' ');
	remarks.push_back(toAdd);
	return fplnData.SetRemarks(vsid::utils::join(remarks).c_str());
}
