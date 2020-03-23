// SPServer.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <thread>
#include <WinSock2.h>
#include <vector>
#include "protocol.h"

#pragma comment(lib, "Ws2_32.lib")

using namespace std;


typedef struct PlayerSockInfo
{
	bool	connected;
	SOCKET	socket;
	char	packet_buf[MAX_BUFFER];

	int		x, y;

}PLAYERSOCKINFO;

typedef struct SockInfo
{
	SOCKET	socket;
	char	packet_buf[MAX_BUFFER];

	int		recvbytes;
	int		sendbytes;

}SOCKINFO;

PLAYERSOCKINFO clients[MAX_USER];

// 오류 출력 함수
void err_quit(char* msg);
void err_display(char* msg);
int recvn(SOCKET s, char* buf, int len, int flags);
void RemoveSocketInfo(int nIndex);
void process_packet(sc_packet_pos pos);
void send_packet(int key, char* packet);
void send_pos_packet(/*char to, char obj*/);
bool AddSocketInfo(SOCKET sock);

int nTotalSockets = 0;
SOCKINFO* SocketInfoArray[FD_SETSIZE];

sc_packet_pos pos;

int g_x, g_y = 0;
bool g_connect = false;

// 소켓 함수 오류 출력 후 종료
void err_quit(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	cout << "erroe - " << (char*)lpMsgBuf << endl;
	LocalFree(lpMsgBuf);
}

int recvn(SOCKET s, char* buf, int len, int flags)      // 고정길이 데이터를 읽기 위해 사용자 정의함수 recvn() 함수 사용 ~62라인까지 - 4장에서 사용
{
	int received;
	char* ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

void RemoveSocketInfo(int nIndex)
{
	SOCKINFO* ptr = SocketInfoArray[nIndex];

	// 클라이언트 정보 얻기
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(ptr->socket, (SOCKADDR*)&clientaddr, &addrlen);
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
		inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

	closesocket(ptr->socket);
	delete ptr;

	if (nIndex != (nTotalSockets - 1))
		SocketInfoArray[nIndex] = SocketInfoArray[nTotalSockets - 1];

	--nTotalSockets;
}

void send_packet(int key, char* packet)
{
	SOCKET client_s = clients[key].socket;

	//OVER_EX* over = reinterpret_cast<OVER_EX*>(malloc(sizeof(OVER_EX)));

	//over->dataBuffer.len = packet[0];
	//over->dataBuffer.buf = over->messageBuffer;
	//memcpy(over->messageBuffer, packet, packet[0]);
	//ZeroMemory(&(over->overlapped), sizeof(WSAOVERLAPPED));
	//over->is_recv = false;
	//if (WSASend(client_s, &over->dataBuffer, 1, NULL,
	//	0, &(over->overlapped), NULL) == SOCKET_ERROR)
	//{
	//	if (WSAGetLastError() != WSA_IO_PENDING)
	//	{
	//		cout << "Error - Fail WSASend(error_code : ";
	//		cout << WSAGetLastError() << ")\n";
	//	}
	//}
}

void send_pos_packet(/*char to, char obj*/)
{
	sc_packet_pos packet;
	//packet.id = obj;
	packet.size = sizeof(packet);
	packet.type = SC_POS;
	packet.x = g_x;
	packet.y = g_y;
	//send_packet(to, reinterpret_cast<char*>(&packet));
}

void process_packet(sc_packet_pos pos)
{
	int iType = pos.type;

	if (iType < 1 || iType >5)
		return;

	switch (iType) {
	case CS_UP:
		if (pos.y > 0)
		{
			pos.y -= BOARD_GAP;
		}
		break;
	case CS_DOWN:
		if (pos.y < BOARD_GAP * 7)
		{
			pos.y += BOARD_GAP;
		}
		break;
	case CS_LEFT: 
		if (0 < pos.x)
		{
			pos.x -= BOARD_GAP;
		}
		break;
	case CS_RIGHT: 
		if ((BOARD_GAP * 7) > pos.x)
		{
			pos.x += BOARD_GAP;
		}
		break;
	default:
		cout << "Unknown Packet Type Error\n";
		break;
	}

	send(SocketInfoArray[0]->socket, (char*)&pos, sizeof(sc_packet_pos), 0);


}

bool AddSocketInfo(SOCKET sock)
{
	if (nTotalSockets >= FD_SETSIZE) {
		printf("[오류] 소켓 정보를 추가할 수 없습니다!\n");
		return FALSE;
	}

	SOCKINFO* ptr = new SOCKINFO;
	if (ptr == NULL) {
		printf("[오류] 메모리가 부족합니다!\n");
		return FALSE;
	}

	ptr->socket = sock;
	ptr->recvbytes = 0;
	ptr->sendbytes = 0;
	SocketInfoArray[nTotalSockets++] = ptr;

	return TRUE;
}

DWORD WINAPI WorkerThread(LPVOID arg)
{
	clock_t Curtime = 0;

	clock_t Oldtime = clock(); // clock() 함수는 프로그램이 시작된 이후의 시간을  밀리세컨드 단위로 알려줌
	auto last_time = chrono::high_resolution_clock::now();
	while (1)
	{
		auto duration_time = chrono::high_resolution_clock::now() - last_time;
		int durationMs = chrono::duration_cast<chrono::milliseconds>(duration_time).count();
		//초당 20번 전송
		if (durationMs > 50)
		{
			process_packet(pos);
			last_time = chrono::high_resolution_clock::now();
		}
	}

	return 0;
}

int main()
{
	int retval = 0;

	HANDLE hThread = NULL;
	hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);

	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
	{
		cout << "Error - Can not load 'winsock.dll' file\n";
		return 0;
	}

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0); // 변경
	if (listenSocket == INVALID_SOCKET)
	{
		cout << "Error - Invalid socket\n";
		return 0;
	}

	// 서버정보 객체설정 // 변경
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(SERVER_PORT);

	//// 2. 소켓설정
	//if (::bind(listenSocket, (struct sockaddr*) & serverAddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	//{
	//	cout << "Error - Fail bind\n";
	//	// 6. 소켓종료
	//	closesocket(listenSocket);
	//	// Winsock End
	//	WSACleanup();
	//	return 0;
	//}

		// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVER_PORT);
	retval = bind(listenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listenSocket, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 넌블로킹 소켓으로 전환
	//********************************************************
	u_long on = 1;
	retval = ioctlsocket(listenSocket, FIONBIO, &on); // 교착상태 해결하기 위함, 여기서 쓸 필요는 없을듯
	if (retval == SOCKET_ERROR) err_display("ioctlsocket()");


	//// 3. 수신대기열생성
	//if (listen(listenSocket, 5) == SOCKET_ERROR)
	//{
	//	cout << "Error - Fail listen\n";
	//	// 6. 소켓종료
	//	closesocket(listenSocket);
	//	// Winsock End
	//	WSACleanup();
	//	return;
	//}

	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR_IN);
	memset(&clientAddr, 0, addrLen);
	SOCKET clientSocket;

	//********************************************************
	FD_SET rset, wset;
	//********************************************************
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen, i;
	int iClientID = -1;


	while (1) {
		// 소켓 셋 초기화
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(listenSocket, &rset); // fd_set은 관찰을 할 소켓들을 등록하는 변수
		for (i = 0; i < nTotalSockets; i++) {
			if (SocketInfoArray[i]->recvbytes > SocketInfoArray[i]->sendbytes)
				FD_SET(SocketInfoArray[i]->socket, &wset);
			else
				FD_SET(SocketInfoArray[i]->socket, &rset);
		}

		// select()
		retval = select(0, &rset, &wset, NULL, NULL); // 대기 시간 0 // 블로킹이던 아니던 신경 안쓰고 여러 소켓 한 스레드로 처리
		if (retval == SOCKET_ERROR) err_quit("select()");

		// 소켓 셋 검사(1): 클라이언트 접속 수용
		if (FD_ISSET(listenSocket, &rset)) { // 변화가 있는지를 확인
			addrlen = sizeof(clientaddr);
			client_sock = accept(listenSocket, (SOCKADDR*)&clientaddr, &addrlen);
			if (client_sock == INVALID_SOCKET) {
				err_display("accept()");
			}
			else {
				u_long off = 0;

				retval = ioctlsocket(client_sock, FIONBIO, &off); // 인자를 off로 하는 이유가? // 교착상태 해결하기 위함, off 매개변수가 0이 아닐 경우 client_sock의 넌블럭킹(비동기) 모드를 활성화 함. off가 0일 경우는 넌블럭킹(비동기) 모드는 비활성화 됨. argp 매개변수는 unsigned long 값을 포인트 함.
				if (retval == SOCKET_ERROR) err_display("ioctlsocket()");

				printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
					inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
				// 소켓 정보 추가
				AddSocketInfo(client_sock);
				g_connect = true;

			}
		}

		// 소켓 셋 검사(2): 데이터 통신
		for (int i = 0; i < nTotalSockets; i++) {
			SOCKINFO* ptr = SocketInfoArray[i];
			if (FD_ISSET(ptr->socket, &rset)) {

				retval = recvn(ptr->socket, (char*)&pos, sizeof(sc_packet_pos), 0); // 키 받음
				//std::cout << a << endl;
				if (pos.type != 0)
				{
					std::cout << (int)pos.size << " , " << (int)pos.type<< " , " << (int)pos.x  << " , " <<(int)pos.y<< endl;
				}
				//send(ptr->sock, (char*)&gameinfo, sizeof(GAMEINFO), 0);

				if (retval == SOCKET_ERROR) {
					err_display("recvKeystate()");
					RemoveSocketInfo(i);
					continue;
				}
			}
		}
	}

	WaitForMultipleObjects(2, &hThread, TRUE, INFINITE);

	closesocket(listenSocket);
	CloseHandle(hThread);

	// 윈속 종료
	WSACleanup();
	return 0;
}

// 로그인 부분 고치기 // 패킷 종류별로 나누기 // id 넣기 