
// EtherRecorderGUIDlg.h : header file
//

#pragma once


// CEtherRecorderGUIDlg dialog
class CEtherRecorderGUIDlg : public CDialogEx
{
// Construction
public:
	CEtherRecorderGUIDlg(CWnd* pParent = nullptr);	// standard constructor

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
public:
	CButton m_cSend;
	CEdit m_cEdit_IP;
	CEdit m_cEdit_PORT;
	CEdit m_cEdit_Commands_Exchanged;
	CEdit m_cEdit_Commands;
	CString m_vEdit_IP;
	CString m_vEdit_Port;
	CString m_vEdit_CommandsExchanged;
	CString m_vEdit_Commands;
};
