// vSID.cpp: Definiert die Initialisierungsroutinen für die DLL.
//

#include "pch.h"
#include "framework.h"
#include "vSID.h"
#include "vSIDPlugin.h"
#include "EuroScopePlugIn.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
//TODO: Wenn diese DLL dynamisch mit MFC-DLLs verknüpft ist,
//		muss für alle aus dieser DLL exportierten Funktionen, die in
//		MFC aufgerufen werden, das AFX_MANAGE_STATE-Makro
//		am Anfang der Funktion hinzugefügt werden.
//
//		Beispiel:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// Hier normaler Funktionsrumpf
//		}
//
//		Es ist sehr wichtig, dass dieses Makro in jeder Funktion
//		vor allen MFC-Aufrufen angezeigt wird.  Dies bedeutet,
//		dass es als erste Anweisung innerhalb der
//		Funktion angezeigt werden muss, sogar vor jeglichen Deklarationen von Objektvariablen,
//		da ihre Konstruktoren Aufrufe in die MFC-DLL generieren
//		könnten.
//
//		Siehe Technische Hinweise für MFC 33 und 58 für weitere
//		Details.
//

// CvSIDApp

BEGIN_MESSAGE_MAP(CvSIDApp, CWinApp)
END_MESSAGE_MAP()


// CvSIDApp-Erstellung

CvSIDApp::CvSIDApp()
{
	// TODO: Hier Code zur Konstruktion einfügen
	// Alle wichtigen Initialisierungen in InitInstance positionieren
}


// Das einzige CvSIDApp-Objekt

CvSIDApp theApp;


// CvSIDApp-Initialisierung

BOOL CvSIDApp::InitInstance()
{
	CWinApp::InitInstance();

	return TRUE;
}

CvSIDPlugin* gpMyPlugin = NULL;

void __declspec (dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn** ppPlugInInstance)
{

	// create the instance
	*ppPlugInInstance = gpMyPlugin = new CvSIDPlugin();
}


//---EuroScopePlugInExit-----------------------------------------------

void __declspec (dllexport) EuroScopePlugInExit(void)
{

	// delete the instance
	delete gpMyPlugin;
}
