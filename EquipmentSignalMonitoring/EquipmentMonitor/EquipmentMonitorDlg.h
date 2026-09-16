
// EquipmentMonitorDlg.h: 헤더 파일
//

#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define WM_RECEIVE_EQUIPMENT_DATA (WM_APP + 1)

struct EquipmentData
{
	double signal;
	int frequency;
	double temperature;
	CString status;
};


// CEquipmentMonitorDlg 대화 상자
class CEquipmentMonitorDlg : public CDialogEx
{
// 생성입니다.
public:
	CEquipmentMonitorDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_EQUIPMENTMONITOR_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
private:
	SOCKET m_clientSocket;
	bool m_isConnected;

	static UINT ReceiveThread(LPVOID pParam);
public:
	afx_msg void OnBnClickedButtonConnect();
	afx_msg LRESULT OnReceiveEquipmentData(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonStop();
};
