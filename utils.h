#pragma once

#include "airport.h"

#include <vector>
#include <string>

//#include "vSID.h"

namespace vsid
{
	namespace utils
	{
		/**
		 * @brief trim spaces on the left side of a string
		 *
		 * @param string
		 * @return std::string
		 */
		std::string ltrim(const std::string& string);
		/**
		 * @brief trim spaces on the right side of a string
		 *
		 * @param string
		 * @return std::string
		 */
		std::string rtrim(const std::string& string);
		/**
		 * @brief trims spaces on both sides of a string
		 *
		 * @param string
		 * @return std::string
		 */
		std::string trim(const std::string& string);
		/**
		 * @brief Splits a string based on a given delimeter
		 *
		 * @param string
		 * @param del - delimiter
		 * @return std::vector<std::string>
		 */
		std::vector<std::string> split(const std::string& string, const char& del);
		/**
		 * @brief Joins a vector of strings into one string
		 * 
		 * @param toJoin - Vector to join 
		 * @param del - delimiter
		 * @return std::string 
		 */
		std::string join(const std::vector<std::string>& toJoin, const char del = ' ');
		/**
		 * @brief Joins a set of strings into one string
		 *
		 * @param toJoin - Vector to join
		 * @param del - delimiter
		 * @return std::string
		 */
		std::string join(const std::set<std::string>& toJoin, const char del = ' ');
		/**
		 * @brief Splits a string based on spaces - splits again for '/' delimeter. Used to vectorize the filed route - WILL BE DELETED
		 *
		 * @param string
		 * @return std::vector<std::string>
		 */
		std::vector<std::string> splitRoute(std::string& string);
		/**
		 * @brief Fixed for airport icao checks - possible change to template
		 *
		 * @param airportVector - vector containing all active airports
		 * @param toSearch - string to search for, should be icao of an airport to check
		 * @return true - if the icao was found in active airports
		 * @return false - if the icao was NOT found in active airports
		 */
		bool isIcaoInVector(const std::vector<vsid::airport>& airportVector, const std::string& toSearch);
		/**
		 * @brief Checks if a number is contained in another number, e.g. if 2 is in 123
		 * 
		 * @param number number to be checked for
		 * @param digit number to be checked in
		 * @return true 
		 * @return false 
		 */
		bool containsDigit(int number, int digit);

		int getMinClimb(int elevation);

		std::string tolower(std::string input);

		template<typename T>
		std::string toupper(T input)
		{
			std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) { return std::toupper(c); });
			return input;
		}
	}
}