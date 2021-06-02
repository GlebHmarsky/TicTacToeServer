
// TicTacToeServerDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "TicTacToeServer.h"
#include "TicTacToeServerDlg.h"
#include "afxdialogex.h"
//using namespace std;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CTicTacToeServerDlg dialog

CTicTacToeServerDlg::CTicTacToeServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_TICTACTOESERVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTicTacToeServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LISTBOX, m_ListBox);
	DDX_Control(pDX, IDC_LOBBYLIST, m_LobbyList);
}

BEGIN_MESSAGE_MAP(CTicTacToeServerDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_START, &CTicTacToeServerDlg::OnBnClickedStart)
	ON_BN_CLICKED(IDC_PRINT, &CTicTacToeServerDlg::OnBnClickedPrint)
	ON_BN_CLICKED(IDC_CANCEL, &CTicTacToeServerDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


#include <winsock2.h>

#define PORT 5150			// ���� �� ���������
#define DATA_BUFSIZE 8192 	// ������ ������ �� ���������
#define MAX_COUNT_LOBBYS 10 // ���������� �����
#define MAX_EVENTS 20 // ���������� �����
int  iPort = PORT; 	 // ���� ��� ������������� �����������
bool bPrint = true; // �������� �� ��������� ��������

typedef struct _SOCKET_INFORMATION {
	CHAR Buffer[DATA_BUFSIZE];
	WSABUF DataBuf;
	SOCKET Socket;
	/*
	DWORD BytesSEND;
	DWORD BytesRECV;*/
} SOCKET_INFORMATION, * LPSOCKET_INFORMATION;

typedef struct _LOBBY_INFORMATION {
	LPSOCKET_INFORMATION FirstOponent;
	LPSOCKET_INFORMATION SecondOponent;
	CString name;
	BOOL FirstOpWannaRematch;
	BOOL SecondOpWannaRematch;
} LOBBY, * LPLOBBY;

BOOL CreateSocketInformation(SOCKET s, char* Str, CListBox* pLB);
void FreeSocketInformation(DWORD Event, char* Str, CListBox* pLB);
LPLOBBY CreateLobbyInformation();
LPLOBBY FindLobbyClient(LPSOCKET_INFORMATION Client);
LPLOBBY GetLobbyByIndex(int index);
void ShowInfoAboutAllLobies();
int SendToClient(char* Str, int length, LPSOCKET_INFORMATION Client);
//void UnhookFromLobbys(LPSOCKET_INFORMATION si);

DWORD EventTotal = 0;
WSAEVENT EventArray[MAX_EVENTS];
LPSOCKET_INFORMATION SocketArray[MAX_EVENTS];

HWND hWnd_LB;  // ��� ������ � ������ �������
HWND hWnd_LobbyL;
UINT ListenThread(PVOID lpParam);

CList<LPLOBBY> LobbyList;

// CTicTacToeServerDlg message handlers

BOOL CTicTacToeServerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	char Str[128];
	sprintf_s(Str, sizeof(Str), "%d", iPort);
	GetDlgItem(IDC_PORT)->SetWindowText(Str);
	if (bPrint)
		((CButton*)GetDlgItem(IDC_PRINT))->SetCheck(1);
	else
		((CButton*)GetDlgItem(IDC_PRINT))->SetCheck(0);

	this->SetWindowTextA("TicTacToe Game Sever");
	return TRUE;  // return TRUE  unless you set the focus to a control
}


// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTicTacToeServerDlg::OnPaint()
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
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTicTacToeServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
/* --------------------		BUTTONS & OTHER EVENTS	 ------------------*/

void CTicTacToeServerDlg::OnBnClickedStart()
{
	char Str[81];

	hWnd_LB = m_ListBox.m_hWnd;   // ��� ListenThread(� � ����� ������ �������)
	hWnd_LobbyL = m_LobbyList.m_hWnd;
	GetDlgItem(IDC_PORT)->GetWindowText(Str, sizeof(Str));
	iPort = atoi(Str);
	if (iPort <= 0 || iPort >= 0x10000)
	{
		AfxMessageBox("Incorrect Port number");
		return;
	}

	AfxBeginThread(ListenThread, NULL);

	GetDlgItem(IDC_START)->EnableWindow(false);
	m_ListBox.AddString("Server started");
}

int SendToSocket(char* Str, int length, SOCKET* socket) {
	if (socket == NULL)
	{
		return 1;
	}
	char SubStr[200];
	WSABUF DataBuffer;
	DataBuffer.buf = SubStr;
	DataBuffer.len = 200;
	LPDWORD DWORDLenght = (LPDWORD)&length;
	CListBox* pLB = (CListBox*)(CListBox::FromHandle(hWnd_LB));
	strcpy(DataBuffer.buf, Str);
	DataBuffer.len = length;
	if (WSASend(*socket,
		&DataBuffer, 1,
		DWORDLenght, 0, NULL, NULL) ==
		SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			sprintf_s(SubStr, sizeof(SubStr),
				"WSASend() failed with "
				"error %d", WSAGetLastError());
			pLB->AddString(SubStr);
			/*FreeSocketInformation(
				Event - WSA_WAIT_EVENT_0, Str, pLB);*/
			return 1;
		}

		// ��������� ������ WSAEWOULDBLOCK. 
		// ������� FD_WRITE ����� ����������, �����
		// � ������ ����� ������ ���������� �����
	}
	return 0;
}

int SendToClient(char* Str, int length, LPSOCKET_INFORMATION Client) {
	if (Client == NULL)
	{
		return 1;
	}
	char SubStr[100];
	LPDWORD DWORDLenght = (LPDWORD)&length;
	CListBox* pLB = (CListBox*)(CListBox::FromHandle(hWnd_LB));
	strcpy(Client->DataBuf.buf, Str);
	Client->DataBuf.len = length;
	if (WSASend(Client->Socket,
		&(Client->DataBuf), 1,
		DWORDLenght, 0, NULL, NULL) ==
		SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			sprintf_s(SubStr, sizeof(SubStr),
				"WSASend() failed with "
				"error %d", WSAGetLastError());
			pLB->AddString(SubStr);
			/*FreeSocketInformation(
				Event - WSA_WAIT_EVENT_0, Str, pLB);*/
			return 1;
		}

		// ��������� ������ WSAEWOULDBLOCK. 
		// ������� FD_WRITE ����� ����������, �����
		// � ������ ����� ������ ���������� �����
	}
	return 0;
}

int SendToOponent(CPoint point, LPSOCKET_INFORMATION Client) {
	LPLOBBY Lobby = FindLobbyClient(Client);
	if (Lobby == NULL)
	{
		return 1;
	}
	/*
	* ����� ������ ���:
	* �����-�� �� ������������ ��������� ���� � ����� ���� �� �������
	* ������! ����������� �������� ������ � ������� ����� �������������� �������� �� ���� ����������� � �����
	* ������� ����� ������ ���� ���-�� ��������� - ������ ������� ����. �� ����� ���������������� �� �� ����� ��������
	*/
	char str[10];
	snprintf(str, 4, "%d %d", point.x, point.y);
	SendToClient(str, 3, Lobby->FirstOponent);
	SendToClient(str, 3, Lobby->SecondOponent);
	return 1;
}

/* --------------------		LOBBY STUFF	 ------------------*/

LPLOBBY CreateLobbyInformation()
{
	LPLOBBY Lobby = new LOBBY();
	char  Str[200];



	if (Lobby == NULL)
	{
		CListBox* pLB = (CListBox*)(CListBox::FromHandle(hWnd_LB));
		sprintf_s(Str, sizeof(Str),
			"new LOBBY() failed with error %d",
			GetLastError());
		pLB->AddString(Str);
		return NULL;
	}
	// ���������� ��������� SocketInfo ��� �������������.
	Lobby->FirstOponent = NULL;
	Lobby->SecondOponent = NULL;
	Lobby->name = *new CString();
	Lobby->FirstOpWannaRematch = false;
	Lobby->SecondOpWannaRematch = false;
	return Lobby;
}

LPLOBBY FindLobbyClient(LPSOCKET_INFORMATION Client) {
	LPLOBBY curLobby;
	for (int i = 0; i < LobbyList.GetCount(); i++)
	{
		curLobby = GetLobbyByIndex(i);
		if (curLobby->FirstOponent == Client ||
			curLobby->SecondOponent == Client)
		{
			return curLobby;
		}
	}
	return NULL;
}

LPLOBBY GetLobbyByIndex(int index) {
	POSITION pos = LobbyList.GetHeadPosition();
	for (int i = 0; pos != NULL; i++)
	{
		if (i == index)
		{
			return LobbyList.GetAt(pos);
		}
		if (pos != NULL)
		{
			LobbyList.GetNext(pos);
		}
	}
	return NULL;
}

int ConnectClienToLobby(int index, LPSOCKET_INFORMATION SocketInfo) {
	LPLOBBY curLobby = GetLobbyByIndex(index);
	if (curLobby->FirstOponent != NULL) {
		curLobby->SecondOponent = SocketInfo;
		ShowInfoAboutAllLobies();
		SendToClient("5", 1, curLobby->FirstOponent);
		SendToClient("6", 1, curLobby->SecondOponent);
		return 0;
	}
	if (curLobby->SecondOponent != NULL) {
		curLobby->FirstOponent = SocketInfo;
		ShowInfoAboutAllLobies();
		SendToClient("5", 1, curLobby->FirstOponent);
		SendToClient("6", 1, curLobby->SecondOponent);
		return 0;
	}
	return 1;
}

int ToggleRematch(LPSOCKET_INFORMATION Client) {
	/*
	* ���� ����� ��� ���� ������ ������
	* ���� �������, �� � ������������� ���� ������ ������� ��� �� ����� ������� ���.
	*/
	LPLOBBY Lobby = FindLobbyClient(Client);
	if (Lobby == NULL)
	{
		return 1;
	}
	if (Lobby->FirstOponent == Client) {
		Lobby->FirstOpWannaRematch = true;
	}
	else if (Lobby->SecondOponent == Client)
	{
		Lobby->SecondOpWannaRematch = true;
	}

	if (Lobby->FirstOpWannaRematch == TRUE &&
		Lobby->SecondOpWannaRematch == TRUE)
	{
		//��� ������� ����� ������, ������� ����� �������� �����-������ ��� �����
		SendToClient("8", 2, Lobby->FirstOponent);
		SendToClient("8", 2, Lobby->SecondOponent);
		Lobby->FirstOpWannaRematch = false;
		Lobby->SecondOpWannaRematch = false;
		Sleep(50);//����� ����� ��������� �� ��������� 
		LPSOCKET_INFORMATION tmpSI = Lobby->FirstOponent;
		Lobby->FirstOponent = Lobby->SecondOponent;
		Lobby->SecondOponent = tmpSI;

		SendToClient("5", 2, Lobby->FirstOponent);
		SendToClient("6", 2, Lobby->SecondOponent);
	}
	return 1;
}

int CreateLobby(CString lobbyName, LPSOCKET_INFORMATION SocketInfo)
{
	LPLOBBY newLobby = CreateLobbyInformation();
	if (newLobby == NULL)
	{
		return 1;//��������� ��� �������� � �������.
	}
	newLobby->FirstOponent = SocketInfo;
	newLobby->SecondOponent = NULL;
	newLobby->name = lobbyName;
	LobbyList.AddTail(newLobby);
	ShowInfoAboutAllLobies();
	return 0;
}

void FreeLobby(LPLOBBY Lobby) {
	LobbyList.RemoveAt(LobbyList.Find(Lobby));
}

int LeaveLobby(LPSOCKET_INFORMATION Client) {
	LPLOBBY Lobby = FindLobbyClient(Client);
	if (Lobby == NULL)
	{
		return 1;
	}
	if (Lobby->FirstOponent == Client &&
		Lobby->SecondOponent == NULL) {

		//������� �����
		FreeLobby(Lobby);
		ShowInfoAboutAllLobies();
		return 0;
	}
	if (Lobby->SecondOponent == Client &&
		Lobby->FirstOponent == NULL)
	{
		//������� �����
		FreeLobby(Lobby);
		ShowInfoAboutAllLobies();
		return 0;
	}

	if (Lobby->FirstOponent == Client &&
		Lobby->SecondOponent != NULL)
	{
		//���������� ������� �������� �� ����� �������
		Lobby->FirstOponent = Lobby->SecondOponent;
		Lobby->SecondOponent = NULL;
		SendToClient("3", 1, Lobby->FirstOponent);
		ShowInfoAboutAllLobies();
		return 0;
	}

	if (Lobby->SecondOponent == Client &&
		Lobby->FirstOponent != NULL)
	{
		//������ ��������� ����� � ������� ���������		
		Lobby->SecondOponent = NULL;
		SendToClient("3", 1, Lobby->FirstOponent);
		ShowInfoAboutAllLobies();
		return 0;
	}

	return 1;
}

void SendLobbysToClient(LPSOCKET_INFORMATION sock_info) {
	char  Str[200];
	LPLOBBY curLobby;
	if (sock_info != NULL) {

		if (SendToClient("1", 1, sock_info))
			return;

		for (int i = 0; i < LobbyList.GetCount(); i++)
		{
			curLobby = GetLobbyByIndex(i);
			if (((curLobby->FirstOponent != NULL ? 1 : 0) + (curLobby->SecondOponent != NULL ? 1 : 0)) > 1)
			{
				continue;//���� ������ ����� ������������ � ������ ������� ��� �����
			}
			snprintf(Str, sizeof(Str),
				"#%d %s's lobby", i + 1, curLobby->name);

			if (SendToClient(Str, sizeof(Str), sock_info))
				break;
			Sleep(70);//��������� ������� ������ ������������ � ������������� � recv �� ������� �������, ��-�� ���� 
		}
	}
	else return;
}

void ShowInfoAboutAllLobies() {
	CListBox* pLB = (CListBox*)(CListBox::FromHandle(hWnd_LobbyL));
	LPLOBBY curLobby;
	pLB->ResetContent();
	char  Str[200];
	for (int i = 0; i < LobbyList.GetCount(); i++)
	{
		curLobby = GetLobbyByIndex(i);

		sprintf_s(Str, sizeof(Str),
			"In lobby #%d locates %d client of 2 And lobby clients is:", i + 1,
			(curLobby->FirstOponent != NULL ? 1 : 0) + (curLobby->SecondOponent != NULL ? 1 : 0));
		pLB->AddString(Str);

		sprintf_s(Str, sizeof(Str), "%d",
			curLobby->FirstOponent != NULL ? curLobby->FirstOponent->Socket : 0);
		pLB->AddString(Str);

		sprintf_s(Str, sizeof(Str), "%d",
			curLobby->SecondOponent != NULL ? curLobby->SecondOponent->Socket : 0);
		pLB->AddString(Str);
	}

	//ADD ��������� ���� ������ ��� ����� �����
	for (int i = 1; i < EventTotal; i++)
	{
		SendLobbysToClient(SocketArray[i]);
	}
}

/* --------------------		SOCKET STUFF	 ------------------*/

BOOL CreateSocketInformation(SOCKET s, char* Str, CListBox* pLB)
{
	LPSOCKET_INFORMATION SI;

	if ((EventArray[EventTotal] = WSACreateEvent()) ==
		WSA_INVALID_EVENT)
	{
		sprintf_s(Str, sizeof(Str),
			"WSACreateEvent() failed with error %d",
			WSAGetLastError());
		pLB->AddString(Str);
		return FALSE;
	}

	if ((SI = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR,
		sizeof(SOCKET_INFORMATION))) == NULL)
	{
		sprintf_s(Str, sizeof(Str),
			"GlobalAlloc() failed with error %d",
			GetLastError());
		pLB->AddString(Str);
		return FALSE;
	}

	// ���������� ��������� SocketInfo ��� �������������.

	char buffer[200];
	SI->Socket = s;
	SI->DataBuf.buf = buffer;

	SocketArray[EventTotal] = SI;
	EventTotal++;
	return(TRUE);
}

void FreeSocketInformation(DWORD Event, char* Str, CListBox* pLB)
{
	LPSOCKET_INFORMATION SI = SocketArray[Event];
	DWORD i;

	closesocket(SI->Socket);
	GlobalFree(SI);
	WSACloseEvent(EventArray[Event]);

	// ������ �������� ������� � �������

	for (i = Event; i < EventTotal; i++)
	{
		EventArray[i] = EventArray[i + 1];
		SocketArray[i] = SocketArray[i + 1];
	}

	EventTotal--;
}

/* --------------------		MAIN THREAD	 ------------------*/

UINT ListenThread(PVOID lpParam)
{
	SOCKET Listen;
	SOCKET Accept;
	SOCKADDR_IN InternetAddr;
	DWORD Event;
	WSANETWORKEVENTS NetworkEvents;
	WSADATA wsaData;
	DWORD Ret;
	DWORD Flags;
	DWORD RecvBytes;
	DWORD SendBytes;
	char  Str[200];
	CListBox* pLB = (CListBox*)(CListBox::FromHandle(hWnd_LB));
	//�������������� ���������� winsock 2
	if ((Ret = WSAStartup(0x0202, &wsaData)) != 0)
	{
		sprintf_s(Str, sizeof(Str),
			"WSAStartup() failed with error %d", Ret);
		pLB->AddString(Str);
		return 1;
	}
	//������ ����� ��� ������������� ������� �� ACCEPT & CLOSE 
	/*
	* ���� ������� �������� 0, ���������� ������� �� ������ ��������� ��������, � ��������� ����� ������� �������� ��� �������������.
	*/
	if ((Listen = socket(AF_INET, SOCK_STREAM, 0)) ==
		INVALID_SOCKET)
	{
		sprintf_s(Str, sizeof(Str),
			"socket() failed with error %d",
			WSAGetLastError());
		pLB->AddString(Str);
		return 1;
	}



	CreateSocketInformation(Listen, Str, pLB);


	//��������� ��� ����� � ��������� �� ������������� ������ � ����������
	if (WSAEventSelect(Listen, EventArray[EventTotal - 1],
		FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
	{
		sprintf_s(Str, sizeof(Str),
			"WSAEventSelect() failed with error %d",
			WSAGetLastError());
		pLB->AddString(Str);
		return 1;
	}

	//��������� ������� IPv4
	InternetAddr.sin_family = AF_INET;
	/*
	* ������� htonl ����������� u_long �� ����� � ������� ������� ������ TCP / IP (������� �������� ������ �������� ������).
	*/
	InternetAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	/*
	* ������� htons ����������� u_short �� ����� � ������� ������� ������ TCP / IP (������� �������� ������ �������� ������).
	*/
	InternetAddr.sin_port = htons(iPort);

	/*
	* ������� bind ��������� ��������� ����� � �������.
	*/
	if (bind(Listen, (PSOCKADDR)&InternetAddr,
		sizeof(InternetAddr)) == SOCKET_ERROR)
	{
		sprintf_s(Str, sizeof(Str),
			"bind() failed with error %d",
			WSAGetLastError());
		pLB->AddString(Str);
		return 1;
	}
	/*
	* ������� listen ��������� ����� � ���������, � ������� �� ������� ��������� ����������.
	*
	* ������ �������� �������� ��:
	* ������������ ����� ������� ��������� �����������. ���� ���������� � SOMAXCONN , �������� ��������� �����,
	* ������������� �� ����� � ��������� ���������� �� ������������ �������� ���������.
	*/
	if (listen(Listen, 5))
	{
		sprintf_s(Str, sizeof(Str),
			"listen() failed with error %d",
			WSAGetLastError());
		pLB->AddString(Str);
		return 1;
	}

	while (TRUE)
	{
		// ���������� ����������� � ������� �� ����� ������

		/*	������� WSAWaitForMultipleEvents ������������, ����� ���� ��� ��� ��������� ������� ������� ��������� � ���������� ���������,
			����� �������� �������� ������� �������� ��� ����� ����������� ������������ ���������� �����-������.

			If the fWaitAll parameter is FALSE, the return value minus WSA_WAIT_EVENT_0
			(!!!) indicates the lphEvents array INDEX(!) of the signaled event object that satisfied the wait.
		*/
		//������� ���������� ����������, ����� ���� �� ������� �������� ������ ������� � ��������� ��������� ���� ������ ����-���
		if ((Event = WSAWaitForMultipleEvents(EventTotal,
			EventArray, FALSE, WSA_INFINITE, FALSE)) ==
			WSA_WAIT_FAILED)
		{
			sprintf_s(Str, sizeof(Str),
				"WSAWaitForMultipleEvents failed"
				" with error %d", WSAGetLastError());
			pLB->AddString(Str);
			return 1;
		}
		/*	������� WSAEnumNetworkEvents ������������ ��������� ������� ������� ��� ���������� ������,
			������� ������ ���������� ������� ������� � ���������� ������� ������� (�������������).
		*/
		if (WSAEnumNetworkEvents(
			SocketArray[Event - WSA_WAIT_EVENT_0]->Socket,
			EventArray[Event - WSA_WAIT_EVENT_0],
			&NetworkEvents) == SOCKET_ERROR)
		{
			sprintf_s(Str, sizeof(Str),
				"WSAEnumNetworkEvents failed"
				" with error %d", WSAGetLastError());
			pLB->AddString(Str);
			return 1;
		}

		if (NetworkEvents.lNetworkEvents & FD_ACCEPT)
		{
			if (NetworkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
			{
				sprintf_s(Str, sizeof(Str),
					"FD_ACCEPT failed with error %d",
					NetworkEvents.iErrorCode[FD_ACCEPT_BIT]);
				pLB->AddString(Str);
				break;
			}

			// ����� ������ ���������� � ���������� ���
			// � ������ ������� � �������

			/*������� accept ��������� ������� ��������� ����������� � ������.

			���� ������ �� ����������, accept ���������� �������� ���� SOCKET, ������� �������� ������������ ��� ������ ������.
			��� ������������ �������� �������� ������������ ������, � �������� ����������� ����������� ����������.

			� ��������� ������ ������������ �������� INVALID_SOCKET,
			� ���������� ��� ������ ����� ��������, ������ WSAGetLastError .*/
			if ((Accept = accept(
				SocketArray[Event - WSA_WAIT_EVENT_0]->Socket,
				NULL, NULL)) == INVALID_SOCKET)
			{
				sprintf_s(Str, sizeof(Str),
					"accept() failed with error %d",
					WSAGetLastError());
				pLB->AddString(Str);
				break;
			}

			// ������� ����� �������. ��������� ����������.
			if (EventTotal >= MAX_EVENTS)
			{
				sprintf_s(Str, sizeof(Str),
					"Too many connections - closing socket: %d", Accept);
				pLB->AddString(Str);

				if (SendToSocket("4", 2, &Accept))
					break;

				closesocket(Accept);
				continue;
			}

			CreateSocketInformation(Accept, Str, pLB);

			//�� �� ����� ��� � ��� Listen
			if (WSAEventSelect(Accept,
				EventArray[EventTotal - 1],
				FD_READ | FD_WRITE | FD_CLOSE) ==
				SOCKET_ERROR)
			{
				sprintf_s(Str, sizeof(Str),
					"WSAEventSelect()failed"
					" with error %d", WSAGetLastError());
				pLB->AddString(Str);
				return 1;
			}

			sprintf_s(Str, sizeof(Str),
				"Socket %d connected", Accept);
			pLB->AddString(Str);
			Sleep(20);
			SendLobbysToClient(SocketArray[EventTotal - 1]);
		}

		// �������� ������ ��� ������ ������, 
		// ���� ��������� ��������������� �������

		if (NetworkEvents.lNetworkEvents & FD_READ ||
			NetworkEvents.lNetworkEvents & FD_WRITE)
		{
			if (NetworkEvents.lNetworkEvents & FD_READ &&
				NetworkEvents.iErrorCode[FD_READ_BIT] != 0)
			{
				sprintf_s(Str, sizeof(Str),
					"FD_READ failed with error %d",
					NetworkEvents.iErrorCode[FD_READ_BIT]);
				pLB->AddString(Str);
				break;
			}

			if (NetworkEvents.lNetworkEvents & FD_WRITE &&
				NetworkEvents.iErrorCode[FD_WRITE_BIT] != 0)
			{
				sprintf_s(Str, sizeof(Str),
					"FD_WRITE failed with error %d",
					NetworkEvents.iErrorCode[FD_WRITE_BIT]);
				pLB->AddString(Str);
				break;
			}

			LPSOCKET_INFORMATION SocketInfo =
				SocketArray[Event - WSA_WAIT_EVENT_0];

			// ������ ������ ������ ���� �������� ����� ����


			SocketInfo->DataBuf.buf = SocketInfo->Buffer;
			SocketInfo->DataBuf.len = DATA_BUFSIZE;

			Flags = 0;

			/*
			* ������� WSARecv �������� ������ �� ������������� ������ ��� ������������ ������ ��� ������������ ����������.
			*
			* ���� ������ �� ��������� � �������� ������ ����������� ����������, WSARecv ���������� ����.
			� ���� ������ ��������� ���������� ����� ��� ������������� ��� ������, ����� ���������� ����� �������� � ��������� ��������������.
			� ��������� ������ ������������ �������� SOCKET_ERROR , � ���������� ��� ������ ����� ��������, ������ WSAGetLastError
			*/

			if (WSARecv(SocketInfo->Socket,
				&(SocketInfo->DataBuf), 1, &RecvBytes,
				&Flags, NULL, NULL) == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					sprintf_s(Str, sizeof(Str),
						"WSARecv()failed with "
						" error %d", WSAGetLastError());
					pLB->AddString(Str);
					FreeSocketInformation(
						Event - WSA_WAIT_EVENT_0, Str, pLB);
					return 1;
				}
			}
			else
			{

				CString str(SocketInfo->Buffer, RecvBytes);

				int nTokenPos = 0;
				int a = -10,
					b = -10;
				CString strToken = str.Tokenize(_T(" "), nTokenPos);
				CString lobbyName;
				//ADD ���-�� ���������� ������� �� ��������� ���������� ��� ������� ���������
				while (!strToken.IsEmpty())
				{
					if (b >= 0)//������� �� �����
					{
						break;
					}
					if (a == -1)//����� �������� ��� ������ �� �������� �����
					{
						lobbyName = strToken;
						break;
					}
					if (a < 0)
						a = atoi(strToken);
					else
						b = atoi(strToken);
					strToken = str.Tokenize(_T("+"), nTokenPos);
				}

				if (b >= 0)//������� ������ �� �����
				{
					CPoint point(a, b);
					//�� � ���� �������� � ��������
					SendToOponent(point, SocketInfo);

				}
				else if (a == -1)
				{
					if (LobbyList.GetCount() >= MAX_COUNT_LOBBYS) {
						SendToClient("9", 2, SocketInfo);
						continue;
					}
					CreateLobby(lobbyName, SocketInfo); //������� �����
				}
				else if (a == -2)
				{
					//rematch
					ToggleRematch(SocketInfo);
				}
				else if (a == -3)
				{
					//leave lobby
					LeaveLobby(SocketInfo);
				}
				else if (a > 0)
				{
					//������������ � ������������� ����� ������� ������� ������� ������ ����� ������
					ConnectClienToLobby(a - 1, SocketInfo);

				}

				// ����� ���������, ���� ���������
				if (bPrint)
				{
					unsigned l = sizeof(Str) - 1;
					if (l > RecvBytes) l = RecvBytes;
					strncpy_s(Str, SocketInfo->Buffer, l);
					Str[l] = 0;
					pLB->AddString(Str);
				}
			}
		}

		if (NetworkEvents.lNetworkEvents & FD_CLOSE)
		{
			if (NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0)
			{
				sprintf_s(Str, sizeof(Str),
					"FD_CLOSE failed with error %d",
					NetworkEvents.iErrorCode[FD_CLOSE_BIT]);
				//pLB->AddString(Str);
			}
			LeaveLobby(SocketArray[Event - WSA_WAIT_EVENT_0]);

			sprintf_s(Str, sizeof(Str),
				"Closing socket information %d",
				SocketArray[Event - WSA_WAIT_EVENT_0]->Socket);
			pLB->AddString(Str);

			FreeSocketInformation(
				Event - WSA_WAIT_EVENT_0, Str, pLB);
		}
	} // while
	return 0;
}

void CTicTacToeServerDlg::OnBnClickedPrint()
{
	if (((CButton*)GetDlgItem(IDC_PRINT))->GetCheck() == 1)
		bPrint = true;
	else
		bPrint = false;
}

void CTicTacToeServerDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
}
