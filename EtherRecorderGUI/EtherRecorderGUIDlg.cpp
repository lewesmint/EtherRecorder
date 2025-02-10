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
	, m_vEditHost(_T(""))
	, m_vEditPort(_T(""))
	, m_vEditLog(_T(""))
	, m_vEditSend(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CEtherRecorderGUIDlg::~CEtherRecorderGUIDlg()
{
}


void CEtherRecorderGUIDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_BUTTON_SEND, m_cSend);
    DDX_Control(pDX, IDC_EDIT_IP, m_cEditHost);
    DDX_Control(pDX, IDC_EDIT_PORT, m_cEditPort);
    DDX_Control(pDX, IDC_COMMANDS_EXCHANGED, m_cEditLog);
    DDX_Control(pDX, IDC_EDIT_COMMANDS, m_cEditSend);
    DDX_Text(pDX, IDC_EDIT_IP, m_vEditHost);
    DDX_Text(pDX, IDC_EDIT_PORT, m_vEditPort);
    DDX_Text(pDX, IDC_COMMANDS_EXCHANGED, m_vEditLog);
    DDX_Text(pDX, IDC_EDIT_COMMANDS, m_vEditSend);
    DDX_Control(pDX, IDC_STATIC_CONNECTION, m_sConnection);
    DDX_Control(pDX, IDC_BUTTON_CONNECT, m_cConnect);
}

BEGIN_MESSAGE_MAP(CEtherRecorderGUIDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_MESSAGE(WM_SOCKET_CONNECTED, &CEtherRecorderGUIDlg::OnSocketConnected)
    ON_MESSAGE(WM_SOCKET_RECEIVE, &CEtherRecorderGUIDlg::OnSocketReceive)
    ON_MESSAGE(WM_SOCKET_CLOSED, &CEtherRecorderGUIDlg::OnSocketClosed)
    ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CEtherRecorderGUIDlg::OnBnClickedButtonConnect)
    ON_COMMAND(ID_FILE_EXIT, &CEtherRecorderGUIDlg::OnFileExit)
    ON_COMMAND(ID_HELP_ABOUT, &CEtherRecorderGUIDlg::OnHelpAbout)
    ON_COMMAND_RANGE(ID_SETLOGLEVEL_TRACE, ID_SETLOGLEVEL_FATAL, &CEtherRecorderGUIDlg::OnSetLogLevel)
    ON_BN_CLICKED(IDC_BUTTON_SEND, &CEtherRecorderGUIDlg::OnBnClickedButtonSend)
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

void CEtherRecorderGUIDlg::OnFileExit()
{
    // For example, close the dialog.
    EndDialog(IDOK);
}

void CEtherRecorderGUIDlg::OnHelpAbout()
{
    AfxMessageBox(_T("EtherRecorderGUI v1.0\nA dialog-based application with menus."));
}


// CEtherRecorderGUIDlg message handlers

BOOL CEtherRecorderGUIDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

    // Initialise the Windows Sockets library.
    if (!AfxSocketInit()) {
        AfxMessageBox(_T("Socket initialisation failed."));
        return FALSE;
    }

    // Create a white brush for the log edit control.
    // m_brLog.CreateSolidBrush(RGB(255, 255, 255));
    // 
    // For black background:
    m_brLog.CreateSolidBrush(RGB(0, 0, 0));

    // Set the parent pointer for our socket.
    m_socket.m_pParent = this;

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    m_cEditHost.SetWindowText(theApp.Get_IP());
    m_cEditPort.SetWindowText(theApp.Get_Port());


    // Load and attach the menu resource.
    if (m_menu.LoadMenu(IDR_MENU1))
    {
        SetMenu(&m_menu);
    }
    else
    {
        AfxMessageBox(_T("Failed to load menu."));
    }


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


HBRUSH CEtherRecorderGUIDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    // Call the base class to get the default brush.
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    // Check if the control is your log edit control.
    if (pWnd->GetDlgCtrlID() == IDC_COMMANDS_EXCHANGED)
    {
        // Set the desired text and background colours.
//        pDC->SetBkColor(RGB(255, 255, 255)); // white background
//        pDC->SetTextColor(RGB(0, 0, 0));       // black text

        pDC->SetBkColor(RGB(0, 0, 0));         // black background
        pDC->SetTextColor(RGB(0, 255, 0));       // green text

        // Return your white brush.
        return m_brLog;
    }

    return hbr;
}

LRESULT CEtherRecorderGUIDlg::OnSocketConnected(WPARAM wParam, LPARAM /*lParam*/)
{
    int nError = static_cast<int>(wParam);
    CString strLog;
    if (nError == 0) {
        strLog = _T("Connected to server.");
		m_sConnection.SetWindowText(_T("Connected"));
		m_cConnect.SetWindowText(_T("Disconnect"));
    }
    else {
        strLog.Format(_T("Connection failed with error: %d"), nError);
    }

    CString existing;
    m_cEditLog.GetWindowText(existing);
    existing += _T("\r\n") + strLog;
    m_cEditLog.SetWindowText(existing);

    return 0;
}

//LRESULT CEtherRecorderGUIDlg::OnSocketReceive(WPARAM /*wParam*/, LPARAM lParam)
//{
//    // lParam holds a pointer to a CString containing the received data.
//    CString* pStr = reinterpret_cast<CString*>(lParam);
//    if (pStr) {
//        CString existing;
//        m_cEditLog.GetWindowText(existing);
//        existing += _T("\r\nReceived: ") + *pStr;
//        m_cEditLog.SetWindowText(existing);
//        delete pStr;
//    }
//    return 0;
//}

void CEtherRecorderGUIDlg::AppendLogText(const CString& strNewText)
{
    // Get current text length.
    int nLength = m_cEditLog.GetWindowTextLength();
    // Set the selection to the end of the text.
    m_cEditLog.SetSel(nLength, nLength);
    // Append the new text.
    m_cEditLog.ReplaceSel(strNewText);
}

LRESULT CEtherRecorderGUIDlg::OnSocketReceive(WPARAM, LPARAM lParam)
{
    CString* pStr = reinterpret_cast<CString*>(lParam);
    if (pStr)
    {
        // Append the received text and add a new line.
        AppendLogText(_T("\r\nReceived: ") + *pStr);
        delete pStr;
    }
    return 0;
}

void CEtherRecorderGUIDlg::ConnectToServer()
{
    CString host;
    CString portStr;
    m_cEditHost.GetWindowText(host);
    m_cEditPort.GetWindowText(portStr);
    UINT port = static_cast<UINT>(_ttoi(portStr));

    // Log the connection attempt.
    CString existing;
    m_cEditLog.GetWindowText(existing);
    existing += _T("\r\nConnecting to ") + host + _T(":") + portStr;
    m_cEditLog.SetWindowText(existing);

    // Create the socket and attempt a connection.
    if (!m_socket.Create()) {
        AfxMessageBox(_T("Socket creation failed."));
        return;
    }
    if (!m_socket.Connect(host, port)) {
        int nError = m_socket.GetLastError();
        // WSAEWOULDBLOCK is normal for non-blocking sockets.
        if (nError != WSAEWOULDBLOCK) {
            AfxMessageBox(_T("Failed to connect."));
        }
    }
}

void CEtherRecorderGUIDlg::SendData()
{
    CString data;
    m_cEditSend.GetWindowText(data);
    int nSent = m_socket.Send(data, data.GetLength());
    if (nSent == SOCKET_ERROR) {
        AfxMessageBox(_T("Failed to send data."));
    }
    else {
        // Log the sent data.
        CString existing;
        m_cEditLog.GetWindowText(existing);
        existing += _T("\r\nSent: ") + data;
        m_cEditLog.SetWindowText(existing);
    }
}

LRESULT CEtherRecorderGUIDlg::OnSocketClosed(WPARAM wParam, LPARAM /*lParam*/)
{
    int nError = static_cast<int>(wParam);
    CString strLog;
    strLog.Format(_T("Socket disconnected (error code: %d)"), nError);

    CString existing;
    m_cEditLog.GetWindowText(existing);
    existing += _T("\r\n") + strLog;
    m_cEditLog.SetWindowText(existing);

    // Optionally, update other UI elements (e.g., disable the send button) here.

    return 0;
}

void CEtherRecorderGUIDlg::OnBnClickedButtonConnect()
{
	if (m_socket.IsConnected()) {
		m_socket.Close();
		m_sConnection.SetWindowTextA("Disconnected");
        m_sConnection.SetWindowTextA("Connect");
	}
	else {
		ConnectToServer();
	}
}

void CEtherRecorderGUIDlg::OnSetLogLevel(UINT nID)
{
    switch (nID)
    {
    case ID_SETLOGLEVEL_TRACE:
		m_cEditSend.SetWindowTextA("log_level=TRACE");
        break;
    case ID_SETLOGLEVEL_DEBUG:
        m_cEditSend.SetWindowTextA("log_level=DEBUG");
        break;
    case ID_SETLOGLEVEL_INFO:
        m_cEditSend.SetWindowTextA("log_level=INFO");
        break;
    case ID_SETLOGLEVEL_NOTICE:
        m_cEditSend.SetWindowTextA("log_level=NOTICE");
        break;
    case ID_SETLOGLEVEL_WARN:
        m_cEditSend.SetWindowTextA("log_level=WARNING");
        break;
    case ID_SETLOGLEVEL_ERROR:
        m_cEditSend.SetWindowTextA("log_level=ERROR");
        break;
    case ID_SETLOGLEVEL_CRITICAL:
        m_cEditSend.SetWindowTextA("log_level=CRITICAL");
        break;
    case ID_SETLOGLEVEL_FATAL:
        m_cEditSend.SetWindowTextA("log_level=FATAL");
        break;
    default:
        break;
    }
}

void CEtherRecorderGUIDlg::OnBnClickedButtonSend()
{
    // TODO: Add your control notification handler code here
    CString command;
    m_cEditSend.GetWindowText(command);
	if (command.IsEmpty()) {
		AfxMessageBox(_T("No command to send."));
		return;
	}
	else {
		m_socket.SendAsMessage(command);
	}
}
