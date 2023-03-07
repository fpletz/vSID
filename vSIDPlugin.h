#pragma once
#include <string>

#include "EuroScopePlugIn.h"
#include "constants.h"

const std::string pluginName = "vSID";
const std::string pluginVersion = "0.1.0";
const std::string pluginAuthor = "Vatger";
const std::string pluginCopyright = "to be selected";
const std::string pluginViewAviso = "";

class CvSIDPlugin : public EuroScopePlugIn::CPlugIn
{
public:
	CvSIDPlugin();
	virtual ~CvSIDPlugin();

	virtual void OnFunctionCall(int FunctionId, const char* sItemString, POINT Pt, RECT Area);
};

