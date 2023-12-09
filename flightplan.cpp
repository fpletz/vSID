#include "pch.h"

#include "flightplan.h"
#include "utils.h"

std::pair<std::string, std::string> vsid::fpln::clean(std::vector<std::string> &filedRoute, std::string origin, std::string filedSidWpt)
{
	std::string sidByController;
	std::string rwyByAtc;

	if (filedRoute.size() > 0)
	{
		if (filedRoute.front().find('/') != std::string::npos && !(filedRoute.front().find("/N") != std::string::npos))
		{
			std::vector<std::string> sidBlock = vsid::utils::split(filedRoute.at(0), '/');
			if (sidBlock.front().find_first_of("0123456789RV") != std::string::npos && sidBlock.front() != origin)
			{
				sidByController = sidBlock.front();
				rwyByAtc = sidBlock.back();
			}
			if (sidBlock.front() == origin)
			{
				rwyByAtc = sidBlock.back();
			}
			filedRoute.erase(filedRoute.begin());
		}
	}
	/* if a possible SID block was found check the entire route for more SIDs (e.g. filed) and erase them as well*/
	if (filedRoute.size() > 0 && filedSidWpt != "")
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
				if (it->substr(0, it->length() - 2) == filedSidWpt)
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
	return { sidByController, rwyByAtc };
}

std::pair<std::string, std::string> vsid::fpln::getAtcBlock(std::vector<std::string>& filedRoute, std::string origin)
{
	std::string sidBlock;
	std::string atcRwy;
	std::string atcSid;
	if (filedRoute.size() > 0)
	{
		if (filedRoute.front().find('/') != std::string::npos && !(filedRoute.front().find("/N") != std::string::npos))
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