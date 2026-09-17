
// EquipmentMonitorDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "EquipmentMonitor.h"
#include "EquipmentMonitorDlg.h"
#include "afxdialogex.h"

#include <string>
#include <cstring>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CEquipmentMonitorDlg 대화 상자



CEquipmentMonitorDlg::CEquipmentMonitorDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_EQUIPMENTMONITOR_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_clientSocket = INVALID_SOCKET;
	m_isConnected = false;
	m_signalThreshold = 0.80;

	m_previousStatus = _T("DISCONNECTED");
	m_isCsvSaving = false;
}

void CEquipmentMonitorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CEquipmentMonitorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DRAWITEM()
	ON_WM_DESTROY()

	ON_BN_CLICKED(
		IDC_BUTTON_CONNECT, 
		&CEquipmentMonitorDlg::OnBnClickedButtonConnect
	)

	ON_MESSAGE(
		WM_RECEIVE_EQUIPMENT_DATA,
		&CEquipmentMonitorDlg::OnReceiveEquipmentData
	)

	ON_BN_CLICKED(
		IDC_BUTTON_START, 
		&CEquipmentMonitorDlg::OnBnClickedButtonStart
	)
	ON_BN_CLICKED(
		IDC_BUTTON_STOP, 
		&CEquipmentMonitorDlg::OnBnClickedButtonStop
	)

	ON_MESSAGE(
		WM_DISCONNECTED,
		&CEquipmentMonitorDlg::OnDisconnected
	)

	ON_BN_CLICKED(
		IDC_BUTTON_SAVE_CSV, 
		&CEquipmentMonitorDlg::OnBnClickedButtonSaveCsv
	)
	ON_BN_CLICKED(
		IDC_BUTTON_APPLY_THRESHOLD, 
		&CEquipmentMonitorDlg::OnBnClickedButtonApplyThreshold
	)

END_MESSAGE_MAP()


// CEquipmentMonitorDlg 메시지 처리기

BOOL CEquipmentMonitorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.
	
	// Connection 초기값
	SetDlgItemText(IDC_EDIT_IP, _T("127.0.0.1"));
	SetDlgItemText(IDC_EDIT_PORT, _T("9000"));

	// Equipment Status 초기값
	SetDlgItemText(IDC_STATIC_SIGNAL, _T("0.00"));
	SetDlgItemText(IDC_STATIC_FREQUENCY, _T("0 Hz"));
	SetDlgItemText(IDC_STATIC_TEMPERATURE, _T("0.0 °C"));
	SetDlgItemText(IDC_STATIC_STATUS, _T("DISCONNECTED"));
	SetDlgItemText(IDC_EDIT_SIGNAL_THRESHOLD, _T("0.80"));

	GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);

	// 그래프 Static을 Owner Draw로 설정
	CWnd* pGraph = GetDlgItem(IDC_STATIC_GRAPH);

	if (pGraph != nullptr)
	{
		pGraph->ModifyStyle(SS_TYPEMASK, SS_OWNERDRAW);
	}

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CEquipmentMonitorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CEquipmentMonitorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CEquipmentMonitorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CEquipmentMonitorDlg::OnBnClickedButtonConnect()
{
	if (m_isConnected)
	{
		AfxMessageBox(_T("Already connected."));
		return;
	}

	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		AfxMessageBox(_T("WSAStartup failed."));
		return;
	}

	m_clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (m_clientSocket == INVALID_SOCKET)
	{
		AfxMessageBox(_T("Socket creation failed."));
		WSACleanup();
		return;
	}

	CString ipText;
	CString portText;

	GetDlgItemText(IDC_EDIT_IP, ipText);
	GetDlgItemText(IDC_EDIT_PORT, portText);

	int port = _ttoi(portText);

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

	CT2A ipAnsi(ipText);

	if (inet_pton(AF_INET, ipAnsi, &serverAddr.sin_addr) != 1)
	{
		AfxMessageBox(_T("Invalid IP address."));

		closesocket(m_clientSocket);
		m_clientSocket = INVALID_SOCKET;
		WSACleanup();

		return;
	}

	int result = connect(
		m_clientSocket,
		reinterpret_cast<sockaddr*>(&serverAddr),
		sizeof(serverAddr)
	);

	if (result == SOCKET_ERROR)
	{
		HandleCommunicationError(
			_T("Connection failed")
		);

		AfxMessageBox(
			_T("Connection failed.")
		);

		closesocket(m_clientSocket);
		m_clientSocket = INVALID_SOCKET;

		WSACleanup();

		return;
	}

	m_isConnected = true;

	AddLog(_T("CONNECTED"));

	AfxBeginThread(
		ReceiveThread,
		this
	);

	SetDlgItemText(IDC_STATIC_STATUS, _T("CONNECTED"));

	GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);

	// 연결 중에는 중복 연결 방지
	GetDlgItem(IDC_BUTTON_CONNECT)->EnableWindow(FALSE);

	// 연결된 주소를 임의로 변경하지 못하도록 비활성화
	GetDlgItem(IDC_EDIT_IP)->EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT_PORT)->EnableWindow(FALSE);

	AfxMessageBox(_T("Connected successfully."));
}

UINT CEquipmentMonitorDlg::ReceiveThread(LPVOID pParam)
{
	CEquipmentMonitorDlg* pDlg =
		static_cast<CEquipmentMonitorDlg*>(pParam);

	char buffer[1024];

	std::string receiveBuffer;

	while (pDlg->m_isConnected)
	{
		int received = recv(
			pDlg->m_clientSocket,
			buffer,
			sizeof(buffer) - 1,
			0
		);

		if (received <= 0)
		{
			break;
		}

		buffer[received] = '\0';

		receiveBuffer += buffer;

		size_t newlinePos;

		while (
			(newlinePos = receiveBuffer.find('\n'))
			!= std::string::npos
			)
		{
			std::string line =
				receiveBuffer.substr(0, newlinePos);

			receiveBuffer.erase(
				0,
				newlinePos + 1
			);

			double signal = 0.0;
			int frequency = 0;
			double temperature = 0.0;

			int parsed = sscanf_s(
				line.c_str(),
				"Signal=%lf;Frequency=%d;Temperature=%lf",
				&signal,
				&frequency,
				&temperature
			);

			if (parsed == 3)
			{
				EquipmentData* data =
					new EquipmentData;

				data->signal = signal;
				data->frequency = frequency;
				data->temperature = temperature;

				pDlg->PostMessage(
					WM_RECEIVE_EQUIPMENT_DATA,
					0,
					reinterpret_cast<LPARAM>(data)
				);
			}
		}
	}

	pDlg->m_isConnected = false;

	pDlg->PostMessage(
		WM_DISCONNECTED,
		0,
		0
	);

	return 0;
}

LRESULT CEquipmentMonitorDlg::OnReceiveEquipmentData(
	WPARAM wParam,
	LPARAM lParam
)
{
	EquipmentData* data =
		reinterpret_cast<EquipmentData*>(lParam);

	if (data == nullptr)
	{
		return 0;
	}

	CString text;

	text.Format(_T("%.2f"), data->signal);
	SetDlgItemText(IDC_STATIC_SIGNAL, text);

	m_signalHistory.push_back(data->signal);

	if (m_signalHistory.size() > 50)
	{
		m_signalHistory.erase(m_signalHistory.begin());
	}

	CWnd* pGraph = GetDlgItem(IDC_STATIC_GRAPH);

	if (pGraph != nullptr)
	{
		pGraph->Invalidate();
	}

	text.Format(_T("%d Hz"), data->frequency);
	SetDlgItemText(IDC_STATIC_FREQUENCY, text);

	text.Format(_T("%.2f °C"), data->temperature);
	SetDlgItemText(IDC_STATIC_TEMPERATURE, text);

	CString currentStatus;

	if (data->signal >= m_signalThreshold)
	{
		currentStatus = _T("WARNING");
	}
	else
	{
		currentStatus = _T("NORMAL");
	}

	SetDlgItemText(
		IDC_STATIC_STATUS,
		currentStatus
	);

	if (currentStatus != m_previousStatus)
	{
		if (currentStatus == _T("WARNING"))
		{
			CString warningLog;

			warningLog.Format(
				_T("WARNING  Signal=%.2f"),
				data->signal
			);

			AddLog(warningLog);
		}
		else if (
			currentStatus == _T("NORMAL") &&
			m_previousStatus == _T("WARNING")
		)
		{
			AddLog(_T("RECOVERED  Status=NORMAL"));
		}

		m_previousStatus = currentStatus;
	}

	if (m_isCsvSaving && m_csvFile.is_open())
	{
		CTime now = CTime::GetCurrentTime();

		CString timeText =
			now.Format(_T("%H:%M:%S"));

		CT2A timeAnsi(timeText);
		CT2A statusAnsi(currentStatus);

		m_csvFile
			<< timeAnsi.m_psz
			<< ","
			<< data->signal
			<< ","
			<< data->frequency
			<< ","
			<< data->temperature
			<< ","
			<< statusAnsi.m_psz
			<< std::endl;
	}

	delete data;

	return 0;
}

void CEquipmentMonitorDlg::OnBnClickedButtonStart()
{
	if (!m_isConnected)
	{
		AfxMessageBox(_T("Not connected."));
		return;
	}

	const char* command = "START\n";

	int result = send(
		m_clientSocket,
		command,
		static_cast<int>(strlen(command)),
		0
	);

	if (result == SOCKET_ERROR)
	{
		HandleCommunicationError(
			_T("START command send failed")
		);

		AfxMessageBox(
			_T("Failed to send START command.")
		);

		return;
	}

	SetDlgItemText(
		IDC_STATIC_STATUS,
		_T("RUNNING")
	);

	AddLog(_T("START"));

	GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(TRUE);
}

void CEquipmentMonitorDlg::OnBnClickedButtonStop()
{
	if (!m_isConnected)
	{
		AfxMessageBox(_T("Not connected."));
		return;
	}

	const char* command = "STOP\n";

	int result = send(
		m_clientSocket,
		command,
		static_cast<int>(strlen(command)),
		0
	);

	if (result == SOCKET_ERROR)
	{
		HandleCommunicationError(
			_T("STOP command send failed")
		);

		AfxMessageBox(
			_T("Failed to send STOP command.")
		);

		return;
	}

	SetDlgItemText(
		IDC_STATIC_STATUS,
		_T("STOPPED")
	);

	AddLog(_T("STOP"));

	GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);
}

void CEquipmentMonitorDlg::OnDrawItem(
	int nIDCtl,
	LPDRAWITEMSTRUCT lpDrawItemStruct
)
{
	// Event Log 그리기
	if (nIDCtl == IDC_LIST_LOG)
	{
		CDC dc;
		dc.Attach(lpDrawItemStruct->hDC);

		CRect rect = lpDrawItemStruct->rcItem;

		if (lpDrawItemStruct->itemID == -1)
		{
			dc.Detach();
			return;
		}

		CListBox* pList =
			static_cast<CListBox*>(
				GetDlgItem(IDC_LIST_LOG)
				);

		CString text;

		pList->GetText(
			lpDrawItemStruct->itemID,
			text
		);

		COLORREF textColor = RGB(0, 0, 0);

		if (text.Find(_T("COMM ERROR")) != -1)
		{
			textColor = RGB(200, 0, 0);
		}
		else if (text.Find(_T("WARNING")) != -1)
		{
			textColor = RGB(220, 120, 0);
		}
		else if (text.Find(_T("RECOVERED")) != -1)
		{
			textColor = RGB(0, 140, 0);
		}

		if (lpDrawItemStruct->itemState & ODS_SELECTED)
		{
			dc.FillSolidRect(
				rect,
				GetSysColor(COLOR_HIGHLIGHT)
			);

			dc.SetTextColor(
				GetSysColor(COLOR_HIGHLIGHTTEXT)
			);
		}
		else
		{
			dc.FillSolidRect(
				rect,
				GetSysColor(COLOR_WINDOW)
			);

			dc.SetTextColor(textColor);
		}

		dc.SetBkMode(TRANSPARENT);

		rect.left += 3;

		dc.DrawText(
			text,
			rect,
			DT_LEFT | DT_VCENTER | DT_SINGLELINE
		);

		dc.Detach();
		return;
	}

	// Real-time Signal
	if (nIDCtl == IDC_STATIC_GRAPH)
	{
		CDC dc;
		dc.Attach(lpDrawItemStruct->hDC);

		CRect rect = lpDrawItemStruct->rcItem;

		// 전체 배경
		dc.FillSolidRect(rect, RGB(255, 255, 255));

		// 실제 그래프가 그려질 영역
		CRect graphRect = rect;
		graphRect.left += 30;     // Y축 숫자 공간
		graphRect.right -= 10;
		graphRect.top += 10;
		graphRect.bottom -= 10;

		// 그래프 테두리
		dc.Rectangle(graphRect);

		CFont smallFont;

		smallFont.CreatePointFont(
			75,              // 7.5pt
			_T("Segoe UI")
		);

		CFont* pOldFont =
			dc.SelectObject(&smallFont);

		// Y축 라벨
		const double yValues[] =
		{
			1.00,
			0.75,
			0.50,
			0.25,
			0.00
		};

		for (double value : yValues)
		{
			int y =
				graphRect.bottom -
				static_cast<int>(
					value * graphRect.Height()
					);

			int labelY = y;

			if (value == 1.00)
				labelY += 6;
			else if (value == 0.75)
				labelY += 4;
			else if (value == 0.25)
				labelY -= 4;
			else if (value == 0.00)
				labelY -= 6;

			CString label;
			label.Format(_T("%.2f"), value);

			CRect labelRect(
				rect.left + 5,
				labelY - 8,
				graphRect.left - 8,
				labelY + 8
			);

			dc.DrawText(
				label,
				&labelRect,
				DT_RIGHT | DT_VCENTER | DT_SINGLELINE
			);
		}

		// 원래 폰트 복구
		dc.SelectObject(pOldFont);

		// 가로 기준선
		CPen gridPen(
			PS_DOT,
			1,
			RGB(200, 200, 200)
		);

		CPen* oldPen =
			dc.SelectObject(&gridPen);

		const double gridValues[] =
		{
			0.25,
			0.50,
			0.75
		};

		for (double value : gridValues)
		{
			int y =
				graphRect.bottom -
				static_cast<int>(
					value * graphRect.Height()
					);

			dc.MoveTo(graphRect.left, y);
			dc.LineTo(graphRect.right, y);
		}

		dc.SelectObject(oldPen);

		// Signal Warning Threshold 선
		int thresholdY =
			graphRect.bottom -
			static_cast<int>(
				m_signalThreshold * graphRect.Height()
				);

		CPen thresholdPen(
			PS_DASH,
			1,
			RGB(220, 140, 0)
		);

		CPen* oldThresholdPen =
			dc.SelectObject(&thresholdPen);

		// Threshold 선 그리기
		dc.MoveTo(graphRect.left, thresholdY);
		dc.LineTo(graphRect.right, thresholdY);

		dc.SelectObject(oldThresholdPen);

		// Threshold 값 표시
		CString thresholdLabel;
		thresholdLabel.Format(
			_T("Threshold %.2f"),
			m_signalThreshold
		);

		CRect thresholdTextRect(
			graphRect.right - 125,
			thresholdY - 16,
			graphRect.right - 12,
			thresholdY
		);

		CFont thresholdFont;
		thresholdFont.CreatePointFont(
			80,                 // 8pt
			_T("Segoe UI")
		);

		CFont* oldThresholdFont =
			dc.SelectObject(&thresholdFont);

		COLORREF oldTextColor =
			dc.SetTextColor(RGB(220, 140, 0));

		dc.DrawText(
			thresholdLabel,
			&thresholdTextRect,
			DT_RIGHT | DT_SINGLELINE
		);

		dc.SetTextColor(oldTextColor);
		dc.SelectObject(oldThresholdFont);

		// Signal 그래프
		if (m_signalHistory.size() >= 2)
		{
			int count =
				static_cast<int>(
					m_signalHistory.size()
					);

			int width = graphRect.Width();
			int height = graphRect.Height();

			const int maxSamples = 50;

			CPen signalPen(
				PS_SOLID,
				2,
				RGB(0, 0, 0)
			);

			CPen* oldSignalPen =
				dc.SelectObject(&signalPen);

			for (int i = 1; i < count; ++i)
			{
				int x1 =
					graphRect.left +
					(i - 1) * width / (maxSamples - 1);

				int x2 =
					graphRect.left +
					i * width / (maxSamples - 1);

				int y1 =
					graphRect.bottom -
					static_cast<int>(
						m_signalHistory[i - 1] * height
						);

				int y2 =
					graphRect.bottom -
					static_cast<int>(
						m_signalHistory[i] * height
						);

				dc.MoveTo(x1, y1);
				dc.LineTo(x2, y2);
			}

			// 최신 Signal 위치 표시
			int lastIndex = count - 1;

			int lastX =
				graphRect.left +
				lastIndex * width / (maxSamples - 1);

			int lastY =
				graphRect.bottom -
				static_cast<int>(
					m_signalHistory[lastIndex] * height
					);

			COLORREF pointColor = RGB(0, 0, 0);

			if (m_signalHistory[lastIndex] >= m_signalThreshold)
			{
				pointColor = RGB(220, 60, 60);
			}

			// 바깥 테두리
			CPen pointPen(
				PS_SOLID,
				2,
				pointColor
			);

			// 내부 채우기
			CBrush pointBrush(pointColor);

			CPen* oldPointPen =
				dc.SelectObject(&pointPen);

			CBrush* oldBrush =
				dc.SelectObject(&pointBrush);

			dc.Ellipse(
				lastX - 4,
				lastY - 4,
				lastX + 4,
				lastY + 4
			);

			dc.SelectObject(oldPointPen);
			dc.SelectObject(oldBrush);
		}

		dc.Detach();
		return;
	}

	// 기본 OnDrawItem
	CDialogEx::OnDrawItem(
		nIDCtl,
		lpDrawItemStruct
	);
}

void CEquipmentMonitorDlg::AddLog(const CString& message)
{
	CTime now = CTime::GetCurrentTime();

	CString logText;
	logText.Format(
		_T("%02d:%02d:%02d  %s"),
		now.GetHour(),
		now.GetMinute(),
		now.GetSecond(),
		message.GetString()
	);

	CListBox* pLog =
		static_cast<CListBox*>(GetDlgItem(IDC_LIST_LOG));

	if (pLog != nullptr)
	{
		pLog->AddString(logText);

		int count = pLog->GetCount();

		if (count > 0)
		{
			pLog->SetCurSel(count - 1);
		}
	}
}

LRESULT CEquipmentMonitorDlg::OnDisconnected(
	WPARAM wParam,
	LPARAM lParam
)
{
	SetDlgItemText(
		IDC_STATIC_STATUS,
		_T("DISCONNECTED")
	);

	GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);

	// 다시 연결할 수 있도록 CONNECT 활성화
	GetDlgItem(IDC_BUTTON_CONNECT)->EnableWindow(TRUE);

	// IP / Port도 다시 수정 가능
	GetDlgItem(IDC_EDIT_IP)->EnableWindow(TRUE);
	GetDlgItem(IDC_EDIT_PORT)->EnableWindow(TRUE);

	// 이전 상태 초기화
	m_previousStatus = _T("DISCONNECTED");

	// 연결 종료 시 CSV 저장도 종료
	if (m_csvFile.is_open())
	{
		m_csvFile.close();
	}

	m_isCsvSaving = false;

	SetDlgItemText(
		IDC_BUTTON_SAVE_CSV,
		_T("SAVE CSV")
	);

	AddLog(_T("DISCONNECTED"));

	if (m_clientSocket != INVALID_SOCKET)
	{
		closesocket(m_clientSocket);
		m_clientSocket = INVALID_SOCKET;
	}

	WSACleanup();

	return 0;
}

void CEquipmentMonitorDlg::OnBnClickedButtonSaveCsv()
{
	if (!m_isCsvSaving)
	{
		CTime now = CTime::GetCurrentTime();

		CString defaultFileName;
		defaultFileName.Format(
			_T("equipment_log_%04d%02d%02d_%02d%02d%02d.csv"),
			now.GetYear(),
			now.GetMonth(),
			now.GetDay(),
			now.GetHour(),
			now.GetMinute(),
			now.GetSecond()
		);

		CFileDialog dlg(
			FALSE,                          // FALSE = 저장
			_T("csv"),                      // 기본 확장자
			defaultFileName,                // 기본 파일명
			OFN_HIDEREADONLY |
			OFN_OVERWRITEPROMPT,
			_T("CSV Files (*.csv)|*.csv||"),
			this
		);

		if (dlg.DoModal() != IDOK)
		{
			return;
		}

		CString filePath = dlg.GetPathName();

		CT2A filePathAnsi(filePath);

		m_csvFile.open(filePathAnsi);

		if (!m_csvFile.is_open())
		{
			AfxMessageBox(_T("CSV file open failed."));
			return;
		}

		m_csvFile
			<< "timestamp,signal,frequency,temperature,status"
			<< std::endl;

		m_isCsvSaving = true;

		SetDlgItemText(
			IDC_BUTTON_SAVE_CSV,
			_T("STOP CSV")
		);

		AddLog(_T("CSV SAVE START"));
	}
	else
	{
		if (m_csvFile.is_open())
		{
			m_csvFile.close();
		}

		m_isCsvSaving = false;

		SetDlgItemText(
			IDC_BUTTON_SAVE_CSV,
			_T("SAVE CSV")
		);

		AddLog(_T("CSV SAVE STOP"));
	}
}

void CEquipmentMonitorDlg::OnBnClickedButtonApplyThreshold()
{
	CString thresholdText;

	GetDlgItemText(
		IDC_EDIT_SIGNAL_THRESHOLD,
		thresholdText
	);

	thresholdText.Trim();

	if (thresholdText.IsEmpty())
	{
		AfxMessageBox(
			_T("Please enter a Signal Level.")
		);
		return;
	}

	const TCHAR* start = thresholdText.GetString();
	TCHAR* end = nullptr;

	double newThreshold = _tcstod(start, &end);

	if (end == start || *end != _T('\0'))
	{
		AfxMessageBox(
			_T("Please enter a numeric value between 0.0 and 1.0.")
		);

		return;
	}

	// Signal Level은 0.0 ~ 1.0 범위
	if (newThreshold < 0.0 || newThreshold > 1.0)
	{
		AfxMessageBox(
			_T("Please enter a Signal Level between 0.0 and 1.0.")
		);

		return;
	}

	m_signalThreshold = newThreshold;

	CWnd* pGraph = GetDlgItem(IDC_STATIC_GRAPH);

	if (pGraph != nullptr)
	{
		pGraph->Invalidate();
	}

	CString logText;

	logText.Format(
		_T("THRESHOLD CHANGED  Signal=%.2f"),
		m_signalThreshold
	);

	AddLog(logText);
}

void CEquipmentMonitorDlg::HandleCommunicationError(
	const CString& message
)
{
	SetDlgItemText(
		IDC_STATIC_STATUS,
		_T("ERROR")
	);

	CString logText;
	logText.Format(
		_T("COMM ERROR  %s"),
		message.GetString()
	);

	AddLog(logText);
}

void CEquipmentMonitorDlg::OnDestroy()
{
	// 수신 스레드 종료 유도
	m_isConnected = false;

	// 소켓 종료
	if (m_clientSocket != INVALID_SOCKET)
	{
		shutdown(m_clientSocket, SD_BOTH);
		closesocket(m_clientSocket);
		m_clientSocket = INVALID_SOCKET;
	}

	// CSV 파일이 열려 있으면 닫기
	if (m_csvFile.is_open())
	{
		m_csvFile.close();
	}

	m_isCsvSaving = false;

	// Winsock 정리
	WSACleanup();

	CDialogEx::OnDestroy();
}
