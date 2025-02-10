
// EtherRecorderGUIDlg.h : header file
//
#pragma once

#include "EtherSocket.h"

// CEtherRecorderGUIDlg dialog
class CEtherRecorderGUIDlg : public CDialogEx
{
// Construction
public:
	CEtherRecorderGUIDlg(CWnd* pParent = nullptr);	// standard constructor

	~CEtherRecorderGUIDlg();
	

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ETHERRECORDERGUI_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	// Custom message handlers for socket notifications.
	afx_msg LRESULT OnSocketConnected(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSocketReceive(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSocketClosed(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnSetLogLevel(UINT nID);	
	afx_msg void OnBnClickedButtonSend();


	// Menu command handlers.
	afx_msg void OnFileExit();
	afx_msg void OnHelpAbout();

	// Optionally, handle system commands if needed.
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);

	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	
public:
	CButton m_sConnection;
	CButton m_cSend;
	CEdit m_cEditHost;
	CEdit m_cEditPort;
	CEdit m_cEditLog;
	CEdit m_cEditSend;
	CString m_vEditHost;
	CString m_vEditPort;
	CString m_vEditLog;
	CString m_vEditSend;

	// Our custom socket instance.
	CEtherSocket m_socket;

	// Functions to connect to the server and send data.
	void ConnectToServer();
	void SendData();
	void AppendLogText(const CString& strNewText);

	CMenu m_menu;
	CButton m_cConnect;
	CBrush m_brLog;
};
