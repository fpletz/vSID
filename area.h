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
		Area(std::vector<std::pair<std::string, std::string>> &coords, bool isActive);
		Area() { this->isActive = false; };

		void showline();
		bool inside(const EuroScopePlugIn::CPosition& fplnPos);
		bool isActive;
	private:
		struct Point
		{
			double lat = 0.0;
			double lon = 0.0;
		};
		std::vector<Point> points;

		using Line = std::pair<Point, Point>;
		std::vector<Line> lines;

		//std::vector<Line> area;

		Point toPoint(std::pair<std::string, std::string> &pos);
		double toDeg(std::string& coord);
	};
}
