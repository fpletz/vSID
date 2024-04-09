/*
vSID is a plugin for the Euroscope controller software on the Vatsim network.
The aim auf vSID is to ease the work of any controller that edits and assigns
SIDs to flightplans.

Copyright (C) 2024 Gameagle (Philip Maier)
Repo @ https://github.com/Gameagle/vSID

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "include/es/EuroScopePlugIn.h"

#include <utility>
#include <string>
#include <vector>

namespace vsid
{
	class Area
	{
		
	public:
		Area(std::vector<std::pair<std::string, std::string>> &coords, bool isActive, bool arrAsDep);
		Area() { 
			this->isActive = false;
			this->arrAsDep = false;
		};

		void showline();
		bool inside(const EuroScopePlugIn::CPosition& fplnPos);
		bool isActive;
		bool arrAsDep;
	private:
		struct Point
		{
			double lat = 0.0;
			double lon = 0.0;
		};
		std::vector<Point> points;

		using Line = std::pair<Point, Point>;
		std::vector<Line> lines;

		Point toPoint(std::pair<std::string, std::string> &pos);
		double toDeg(std::string& coord);
	};
}
