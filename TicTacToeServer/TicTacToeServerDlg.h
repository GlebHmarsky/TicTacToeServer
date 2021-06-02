
// TicTacToeServerDlg.h : header file
//

#pragma once


// CTicTacToeServerDlg dialog
class CTicTacToeServerDlg : public CDialog
{
// Construction
public:
	CTicTacToeServerDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TICTACTOESERVER_DIALOG };
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
	afx_msg void OnBnClickedStart();
	CListBox m_ListBox;
	afx_msg void OnBnClickedPrint();
	CListBox m_LobbyList;
	afx_msg void OnBnClickedCancel();
};
