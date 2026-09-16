
// EquipmentMonitorDlg.h: 헤더 파일
//

#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

#define WM_RECEIVE_EQUIPMENT_DATA (WM_APP + 1)
#define WM_DISCONNECTED (WM_APP + 2)

struct EquipmentData
{
	double signal = 0.0;
	int frequency = 0;
	double temperature = 0.0;
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
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
private:
	SOCKET m_clientSocket;
	bool m_isConnected;

	double m_temperatureThreshold;
	
	std::vector<double> m_signalHistory;

	static UINT ReceiveThread(LPVOID pParam);
	void AddLog(const CString& message);
	void HandleCommunicationError(const CString& message);

	CString m_previousStatus;

	std::ofstream m_csvFile;
	bool m_isCsvSaving;
public:
	afx_msg void OnBnClickedButtonConnect();
	afx_msg LRESULT OnReceiveEquipmentData(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg LRESULT OnDisconnected(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedButtonSaveCsv();
	afx_msg void OnBnClickedButtonApplyThreshold();
};
