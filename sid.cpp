#include "pch.h"
#include "sid.h"
#include "utils.h"

std::string vsid::Sid::name() const
{
	return (this->empty()) ? "" : this->waypoint + this->number + this->designator[0];
}

std::string vsid::Sid::fullName() const
{
	return (this->empty()) ? "" : this->waypoint + this->number + this->designator;
}

std::string vsid::Sid::getRwy() const
{
	if (this->rwy.find(",") != std::string::npos)
	{
		return vsid::utils::split(this->rwy, ',').front();
	}
	else return this->rwy;
}

bool vsid::Sid::empty() const
{
	return waypoint == "";
}

bool vsid::Sid::operator==(const Sid& sid)
{
	if (this->waypoint == sid.waypoint &&
		this->number == sid.number &&
		this->designator[0] == sid.designator[0]
		)
	{
		return true;
	}
	else return false;
}

bool vsid::Sid::operator!=(const Sid& sid)
{
	if (this->waypoint != sid.waypoint ||
		this->number != sid.number ||
		this->designator[0] != sid.designator[0]
		)
	{
		return true;
	}
	else return false;
}
