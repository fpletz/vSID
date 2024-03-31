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
