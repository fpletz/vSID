#pragma once

#include <string>
#include <chrono>
#include <format>
namespace vsid
{
	namespace time
	{
		/**
		 * @brief Checks if a given SID time restriction is between start and end time
		 * 
		 * @param timezone - which timezone should be checked
		 * @param start - start time
		 * @param end - end time
		 * @return if the restricted SID is active
		 */
		bool isActive(const std::string& timezone, const int start, const int end);

		/**
		 * @brief Get the current time in utc (ceiled in seconds)
		 * 
		 */
		std::chrono::time_point<std::chrono::utc_clock, std::chrono::seconds> getUtcNow();

		/**
		 * @brief Transform a timepoint into a string
		 * 
		 * @tparam T - the clock e.g. utc_clock
		 * @tparam U - time resolutiong e.g. ::seconds
		 * @param timePoint 
		 * 
		 */
		template<typename T, typename U>
		std::string toFullString(const std::chrono::time_point<T, U>& timePoint)
		{
			return std::string(std::format("{:%Y.%m.%d %H:%M:%S}", timePoint));
		}

		template<typename T, typename U>
		std::string toTimeString(const std::chrono::time_point<T, U>& timePoint)
		{
			return std::string(std::format("{:%H:%M:%S}", timePoint));
		}
	}
}


