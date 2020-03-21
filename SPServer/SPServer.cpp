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

PLAYERSOCKINFO clients[MAX_USER];

// 오류 출력 함수
void err_quit(char* msg);
void err_display(char* msg);
void process_packet(char id, char* buf);
void send_packet(int key, char* packet);
void send_pos_packet(char to, char obj);

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

void send_pos_packet(char to, char obj)
{
	sc_packet_pos packet;
	packet.id = obj;
	packet.size = sizeof(packet);
	packet.type = SC_POS;
	packet.x = clients[obj].x;
	packet.y = clients[obj].y;
	send_packet(to, reinterpret_cast<char*>(&packet));
}

void process_packet(char id, char* buf)
{
	cs_packet_up* packet = reinterpret_cast<cs_packet_up*>(buf);

	char x = clients[id].x;
	char y = clients[id].y;
	switch (packet->type) {
	case CS_UP:
		y-=BOARD_GAP;
		if (y < 0)
			y = 0;
		break;
	case CS_DOWN:
		y += BOARD_GAP;
		if (y >= WORLD_HEIGHT) 
			y = WORLD_HEIGHT - BOARD_GAP;
		break;
	case CS_LEFT: 
		if (0 < x) 
			x -= BOARD_GAP; 
		break;
	case CS_RIGHT: 
		if ((WORLD_WIDTH - BOARD_GAP) > x)
			x += BOARD_GAP;
		break;
	default:
		cout << "Unknown Packet Type Error\n";
		while (true);
	}
	clients[id].x = x;
	clients[id].y = y;

	for (int i = 0; i < MAX_USER; ++i)
	{
		if (true == clients[i].connected)
			send_pos_packet(i, id);
	}
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
			//CalKeyInput(player, Curtime);
			last_time = chrono::high_resolution_clock::now();
		}
	}

	return 0;
}

void worker_thread()
{
	while (true) {
		//DWORD io_byte;
		//ULONG key;
		//OVER_EX* lpover_ex;
		//BOOL is_error = GetQueuedCompletionStatus(g_iocp, &io_byte, (PULONG_PTR)&key,
		//	reinterpret_cast<LPWSAOVERLAPPED*>(&lpover_ex), INFINITE);

		//if (FALSE == is_error)
		//	error_display("GQCS ", WSAGetLastError());
		//if (0 == io_byte) disconnect(key);

		//if (lpover_ex->is_recv) {
		//	int rest_size = io_byte;
		//	char* ptr = lpover_ex->messageBuffer;
		//	char packet_size = 0;
		//	if (0 < clients[key].prev_size) packet_size = clients[key].packet_buf[0];
		//	while (rest_size > 0) {
		//		if (0 == packet_size) packet_size = ptr[0];
		//		int required = packet_size - clients[key].prev_size;
		//		if (rest_size >= required) {
		//			memcpy(clients[key].packet_buf + clients[key].prev_size, ptr, required);
		//			process_packet(key, clients[key].packet_buf);
		//			rest_size -= required;
		//			ptr += required;
		//			packet_size = 0;
		//		}
		//		else {
		//			memcpy(clients[key].packet_buf + clients[key].prev_size,ptr, rest_size);
		//			rest_size = 0;
		//		}
		//	}
		//	do_recv(key);
		//}
		//else {
		//	delete lpover_ex;
		//}
	}
}

void do_accept()
{
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
	{
		cout << "Error - Can not load 'winsock.dll' file\n";
		return;
	}

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0); // 변경
	if (listenSocket == INVALID_SOCKET)
	{
		cout << "Error - Invalid socket\n";
		return;
	}

	// 서버정보 객체설정 // 변경
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(SERVER_PORT);

	// 2. 소켓설정
	if (::bind(listenSocket, (struct sockaddr*) & serverAddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		cout << "Error - Fail bind\n";
		// 6. 소켓종료
		closesocket(listenSocket);
		// Winsock End
		WSACleanup();
		return;
	}

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
	//DWORD flags;

	while (true)
	{
		clientSocket = accept(listenSocket, (struct sockaddr*) & clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
		{
			cout << "Error - Accept Failure\n";
			return;
		}

		//memset(&clients[0], NULL, sizeof(PLAYERSOCKINFO));
		//clients[0].socket = clientSocket;
		//clients[0].over.dataBuffer.len = MAX_BUFFER;
		//clients[0].over.dataBuffer.buf = clients[clientSocket].over.messageBuffer;
		//clients[0].over.is_recv = true;
		clients[0].x = START_X;
		clients[0].y = START_Y;
		//flags = 0;

		//CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket), g_iocp, new_id, 0);
		clients[0].connected = true;

		//send_login_ok_packet(new_id);
		//for (int i = 0; i < MAX_USER; ++i) {
		//	if (false == clients[i].connected) continue;
		//	send_put_player_packet(i, new_id);
		//}
		//for (int i = 0; i < MAX_USER; ++i) {
		//	if (false == clients[i].connected) continue;
		//	if (i == new_id) continue;
		//	send_put_player_packet(new_id, i);
		//}
		//do_recv(new_id);
	}

	// 6-2. 리슨 소켓종료
	closesocket(listenSocket);

	// Winsock End
	WSACleanup();

	return;
}

int main()
{
	HANDLE hThread = NULL;

	vector<thread> Worker_Threads;

	//hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);

	for (auto& client : clients) {
		client.connected = false;
	}

	for (int i = 0; i < MAX_USER; ++i)
	{
		Worker_Threads.emplace_back(thread{ worker_thread });
	}

	thread accept_thread{ do_accept };

	for (auto& thread : Worker_Threads)
		thread.join();

	CloseHandle(hThread);
}
