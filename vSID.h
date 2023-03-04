// vSID.h: Hauptheaderdatei für die vSID-DLL
//

#pragma once
#include "EuroScopePlugIn.h"
#include "vSIDPlugin.h"
#ifndef __AFXWIN_H__
	#error "'pch.h' vor dieser Datei für PCH einschließen"
#endif

#include "resource.h"		// Hauptsymbole


// CvSIDApp
// Informationen zur Implementierung dieser Klasse finden Sie unter vSID.cpp.
//

class CvSIDApp : public CWinApp
{
public:
	CvSIDApp();

// Überschreibungen
public:
	virtual BOOL InitInstance();
	CvSIDPlugin * gpMyPlugin = NULL;
	DECLARE_MESSAGE_MAP()
};
