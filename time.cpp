#include "pch.h"
#include "time.h"

#include "messageHandler.h"

#include <chrono>

bool vsid::time::isActive(const std::string& timezone, const int start, const int end)
{
	try
	{
		std::chrono::zoned_time zt{ timezone, std::chrono::system_clock::now() };
		std::chrono::zoned_time ztStart = zt;
		std::chrono::zoned_time ztEnd = zt;

		auto day = std::chrono::floor<std::chrono::days>(zt.get_local_time());
		ztStart = day + std::chrono::hours{ start };
		ztEnd = day + std::chrono::hours{ end };

		if ((ztEnd.get_local_time() < ztStart.get_local_time() &&
			(zt.get_local_time() > ztStart.get_local_time() || zt.get_local_time() < ztEnd.get_local_time())) ||
			ztEnd.get_local_time() > ztStart.get_local_time() &&
			zt.get_local_time() > ztStart.get_local_time() &&
			zt.get_local_time() < ztEnd.get_local_time()
			)
		//if(zt.get_local_time() > ztStart.get_local_time() || zt.get_local_time() < ztEnd.get_local_time())
		{
			std::ostringstream ss;
			ss << zt.get_local_time() << " vs. start: " << ztStart.get_local_time() << " vs. end: " << ztEnd.get_local_time();
			messageHandler->writeMessage("DEBUG", "time now: " + ss.str());
			return true;
		}
	}
	catch (std::runtime_error& e)
	{
		messageHandler->writeMessage("ERROR", "Timezone failed - " + std::string(e.what()) + ": " + timezone);
	}
	return false;
}
