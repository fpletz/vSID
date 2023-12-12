#include "pch.h"
#include "sid.h"
#include "utils.h"

std::string vsid::sids::getName(const sid &sid)
{
	return (isEmpty(sid)) ? "" : sid.waypoint + sid.number + sid.designator[0];
}

std::string vsid::sids::getRwy(const sid& sid)
{
	if (sid.rwy.find(",") != std::string::npos)
	{
		return vsid::utils::split(sid.rwy, ',').front();
	}
	else return sid.rwy;
}

bool vsid::sids::isEmpty(const sid& sid)
{
	return sid.waypoint == "";
}

bool vsid::sids::operator==(const sid& sid1, const sid& sid2)
{
	if (sid1.waypoint == sid2.waypoint &&
		sid1.number == sid2.number &&
		sid1.designator[0] == sid2.designator[0]
		)
	{
		return true;
	}
	else return false;
}

bool vsid::sids::operator!=(const sid& sid1, const sid& sid2)
{
	if (sid1.waypoint != sid2.waypoint ||
		sid1.number != sid2.number ||
		sid1.designator[0] != sid2.designator[0]
		)
	{
		return true;
	}
	else return false;
}
