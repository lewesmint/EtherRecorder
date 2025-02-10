
// EtherRecorderGUI.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CEtherRecorderGUIApp:
// See EtherRecorderGUI.cpp for the implementation of this class
//

class CEtherRecorderGUIApp : public CWinApp
{
public:
	CEtherRecorderGUIApp();

// Overrides
public:
	virtual BOOL InitInstance();
	CString Get_IP();
	CString Get_Port();

// Implementation

	DECLARE_MESSAGE_MAP()
private:
	CString m_szIP;
	CString m_szPort;
	BOOL LoadProfile();
public:
};

extern CEtherRecorderGUIApp theApp;
