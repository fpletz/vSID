#include "pch.h"
#include "vSIDPlugin.h"

CvSIDPlugin::CvSIDPlugin() : EuroScopePlugIn::CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, pluginName.c_str(), pluginVersion.c_str(), pluginAuthor.c_str(), pluginCopyright.c_str()) {

	RegisterTagItemType("Check SID", TAG_ITEM_VSID_CHECK);

	DisplayUserMessage(pluginName.c_str(), "", std::string("Version " + pluginVersion + " loaded").c_str(), true, true, false, false, false);
}

CvSIDPlugin::~CvSIDPlugin() {}

void CvSIDPlugin::OnFunctionCall(int FunctionId, const char * sItemString, POINT Pt, RECT Area) {

}
