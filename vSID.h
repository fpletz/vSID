// vSID.h: Hauptheaderdatei für die vSID-DLL
//

#pragma once
#include "EuroScopePlugIn.h"
#include "vSIDPlugin.h"

//#include "utils.h"
#ifndef __AFXWIN_H__
	#error "'pch.h' vor dieser Datei für PCH einschließen"
#endif

#include "resource.h"		// Hauptsymbole


// CvSIDApp
// Informationen zur Implementierung dieser Klasse finden Sie unter vSID.cpp.
//

/**
 * @brief DLL - main app for the plugin
 * 
 */
namespace vsid
{
	//class VSIDPlugin;
	class VSIDApp : public CWinApp
	{
	public:
		VSIDApp();

		// Überschreibungen
	public:
		virtual BOOL InitInstance();
		/**
		 * @brief pointer to the plugin for ES
		 *
		 */
		//vsid::VSIDPlugin* gpMyPlugin = NULL;
		//vsid::VSIDPlugin* vsidPlugin = NULL;
		DECLARE_MESSAGE_MAP()
	};
}
