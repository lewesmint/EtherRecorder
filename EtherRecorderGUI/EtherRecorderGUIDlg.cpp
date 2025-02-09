
// EtherRecorderGUIDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "EtherRecorderGUI.h"
#include "EtherRecorderGUIDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CEtherRecorderGUIDlg dialog



CEtherRecorderGUIDlg::CEtherRecorderGUIDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ETHERRECORDERGUI_DIALOG, pParent)
	, m_vEdit_IP(_T(""))
	, m_vEdit_Port(_T(""))
	, m_vEdit_CommandsExchanged(_T(""))
	, m_vEdit_Commands(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CEtherRecorderGUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_SEND, m_cSend);
	DDX_Control(pDX, IDC_EDIT_IP, m_cEdit_IP);
	DDX_Control(pDX, IDC_EDIT_PORT, m_cEdit_PORT);
	DDX_Control(pDX, IDC_COMMANDS_EXCHANGED, m_cEdit_Commands_Exchanged);
	DDX_Control(pDX, IDC_EDIT_COMMANDS, m_cEdit_Commands);
	DDX_Text(pDX, IDC_EDIT_IP, m_vEdit_IP);
	DDX_Text(pDX, IDC_EDIT_PORT, m_vEdit_Port);
	DDX_Text(pDX, IDC_COMMANDS_EXCHANGED, m_vEdit_CommandsExchanged);
	DDX_Text(pDX, IDC_EDIT_COMMANDS, m_vEdit_Commands);
}

BEGIN_MESSAGE_MAP(CEtherRecorderGUIDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()


// CEtherRecorderGUIDlg message handlers

BOOL CEtherRecorderGUIDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_cEdit_IP.SetWindowText(theApp.Get_IP());
	m_cEdit_PORT.SetWindowText(theApp.Get_Port()); 


	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CEtherRecorderGUIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CEtherRecorderGUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

