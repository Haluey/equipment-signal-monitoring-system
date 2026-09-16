
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
	m_temperatureThreshold = 37.0;

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
	SetDlgItemText(IDC_EDIT_TEMP_THRESHOLD, _T("37.0"));

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
			char status[32]{};

			int parsed = sscanf_s(
				line.c_str(),
				"Signal=%lf;Frequency=%d;Temperature=%lf;Status=%31s",
				&signal,
				&frequency,
				&temperature,
				status,
				static_cast<unsigned>(_countof(status))
			);

			if (parsed == 4)
			{
				EquipmentData* data =
					new EquipmentData;

				data->signal = signal;
				data->frequency = frequency;
				data->temperature = temperature;
				data->status = CString(status);

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

	if (data->temperature >= m_temperatureThreshold)
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
				_T("WARNING  Temp=%.2f °C"),
				data->temperature
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
	if (nIDCtl == IDC_STATIC_GRAPH)
	{
		CDC dc;
		dc.Attach(lpDrawItemStruct->hDC);

		CRect rect = lpDrawItemStruct->rcItem;

		// 배경 지우기
		dc.FillSolidRect(rect, RGB(255, 255, 255));

		// 그래프 테두리
		dc.Rectangle(rect);

		// 그래프 안쪽 여백
		CRect graphRect = rect;
		graphRect.DeflateRect(10, 10);

		// 그래프 기준선
		CPen gridPen(
			PS_DOT,
			1,
			RGB(200, 200, 200)
		);

		CPen* pOldPen = dc.SelectObject(&gridPen);

		int y25 = graphRect.bottom -
			static_cast<int>(0.25 * graphRect.Height());

		int y50 = graphRect.bottom -
			static_cast<int>(0.50 * graphRect.Height());

		int y75 = graphRect.bottom -
			static_cast<int>(0.75 * graphRect.Height());

		dc.MoveTo(graphRect.left, y25);
		dc.LineTo(graphRect.right, y25);

		dc.MoveTo(graphRect.left, y50);
		dc.LineTo(graphRect.right, y50);

		dc.MoveTo(graphRect.left, y75);
		dc.LineTo(graphRect.right, y75);

		dc.SelectObject(pOldPen);

		if (m_signalHistory.size() >= 2)
		{
			int count =
				static_cast<int>(m_signalHistory.size());

			int width = graphRect.Width();
			int height = graphRect.Height();

			CPen signalPen(
				PS_SOLID,
				2,
				RGB(0, 0, 0)
			);

			CPen* pOldSignalPen =
				dc.SelectObject(&signalPen);

			for (int i = 1; i < count; ++i)
			{
				double previousValue =
					m_signalHistory[i - 1];

				double currentValue =
					m_signalHistory[i];

				int x1 =
					graphRect.left +
					(i - 1) * width / (count - 1);

				int x2 =
					graphRect.left +
					i * width / (count - 1);

				int y1 =
					graphRect.bottom -
					static_cast<int>(
						previousValue * height
						);

				int y2 =
					graphRect.bottom -
					static_cast<int>(
						currentValue * height
						);

				dc.MoveTo(x1, y1);
				dc.LineTo(x2, y2);
			}

			dc.SelectObject(pOldSignalPen);
		}

		dc.Detach();

		return;
	}

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
		IDC_EDIT_TEMP_THRESHOLD,
		thresholdText
	);

	double newThreshold = _ttof(thresholdText);

	if (newThreshold <= 0.0)
	{
		AfxMessageBox(
			_T("Please enter a valid temperature threshold.")
		);

		return;
	}

	m_temperatureThreshold = newThreshold;

	CString logText;

	logText.Format(
		_T("THRESHOLD CHANGED  Temp=%.1f C"),
		m_temperatureThreshold
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
