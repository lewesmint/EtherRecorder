
// EtherRecorderGUI.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "framework.h"
#include "EtherRecorderGUI.h"
#include "EtherRecorderGUIDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CEtherRecorderGUIApp

BEGIN_MESSAGE_MAP(CEtherRecorderGUIApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CEtherRecorderGUIApp construction

CEtherRecorderGUIApp::CEtherRecorderGUIApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CEtherRecorderGUIApp object

CEtherRecorderGUIApp theApp;


BOOL CEtherRecorderGUIApp::LoadProfile()
{
	// Get the current working directory.
	char cwd[MAX_PATH] = { 0 };
	if (GetCurrentDirectoryA(MAX_PATH, cwd) == 0)
	{
		AfxMessageBox("Failed to get current working directory.");
		return FALSE;
	}

	// Build the full path to your INI file, e.g. "C:\YourDir\config.ini"
	CString iniPath;
	iniPath.Format("%s\\gui_config.ini", cwd);

	// Allocate a copy of the path so that m_pszProfileName points to valid memory.
	// (MFC will use m_pszProfileName when reading/writing profile settings.)
	m_pszProfileName = _strdup((LPCTSTR)iniPath);
	if (m_pszProfileName == NULL)
	{
		AfxMessageBox("Failed to allocate memory for the profile name.");
		return FALSE;
	}

	// Now, any calls to WriteProfileString will write to the INI file at the path you specified.
//	WriteProfileString("Settings", "Key", "Hello");

	m_szIP = GetProfileString("network", "IP", "localhost");
	m_szPort = GetProfileString("network", "port", "4999");

	// You can read it back later with GetProfileString.

	printf("Loaded IP: %s\n", (LPCTSTR)m_szIP);
	printf("Loaded port: %s\n", (LPCTSTR)m_szPort);

	// Continue with the rest of your initialization...
	return TRUE;
}

// CEtherRecorderGUIApp initialization

BOOL CEtherRecorderGUIApp::InitInstance()
{
	LoadProfile();

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CEtherRecorderGUIDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
		TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}

	// Delete the shell manager created above.
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}


CString CEtherRecorderGUIApp::Get_IP() {
	return m_szIP;
}

CString CEtherRecorderGUIApp::Get_Port() {
	return m_szPort;
}
