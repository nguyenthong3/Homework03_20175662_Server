#include "stdafx.h"
#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <conio.h>
#include "handreq.h"
#include "handresp.h"
#include "readtext.h"
#include "session.h"
#include "thread.h"

#pragma comment (lib,"ws2_32.lib")
#pragma warning (disable: 4996)

#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048

using namespace std;

void processData(char*, char*);
int Receive(SOCKET, char*, int, int);
int Send(SOCKET, char*, int, int);

int main(int argc, TCHAR* argv[])
{
	//Check input cmd [Filename.exe PortNumber]
	if (argc != 2 || atoi((char*)argv[1]) < 49152 || atoi((char*)argv[1]) > 65535) {
		cout << "Error input (notice: Port must in [49152;65535])" << endl;
		return 0;
	}

	//Step 1: Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported\n");

	//Step 2: Construct socket	
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi((char*)argv[1]));
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)))
	{
		printf("Error! Cannot bind this address.");
		_getch();
		return 0;
	}

	//Step 4: Listen request from client
	if (listen(listenSock, 10)) {
		printf("Error! Cannot listen.");
		_getch();
		return 0;
	}

	printf("Server started!\n");


	SOCKET client[FD_SETSIZE], connSock;
	fd_set readfds, initfds; //use initfds to initiate readfds at the begining of every loop step
	sockaddr_in clientAddr;
	int ret, nEvents, clientAddrLen;
	char rcvBuff[BUFF_SIZE], sendBuff[BUFF_SIZE];

	for (int i = 0; i < FD_SETSIZE; i++)
		client[i] = 0;	// 0 indicates available entry

	FD_ZERO(&initfds);
	FD_SET(listenSock, &initfds);

	//Step 5: Communicate with clients
	while (1) {
		readfds = initfds;		/* structure assignment */
		nEvents = select(0, &readfds, 0, 0, 0);
		if (nEvents < 0) {
			printf("\nError! Cannot poll sockets: %d", WSAGetLastError());
			break;
		}

		//new client connection
		if (FD_ISSET(listenSock, &readfds)) {
			clientAddrLen = sizeof(clientAddr);
			if ((connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen)) < 0) {
				printf("\nError! Cannot accept new connection: %d", WSAGetLastError());
				break;
			}
			else {
				printf("You got a connection from %s\n", inet_ntoa(clientAddr.sin_addr)); /* prints client's IP */
				printf("%d\n", ntohs(clientAddr.sin_port));

				int i;
				for (i = 0; i < FD_SETSIZE; i++)
					if (client[i] == 0) {
						client[i] = connSock;
						FD_SET(client[i], &initfds);
						break;
					}

				if (i == FD_SETSIZE) {
					printf("\nToo many clients.");
					closesocket(connSock);
				}

				if (--nEvents == 0)
					continue; //no more event
			}
		}

		//receive data from clients
		for (int i = 0; i < FD_SETSIZE; i++) {
			if (client[i] == 0)
				continue;

			if (FD_ISSET(client[i], &readfds)) {
				ret = Receive(client[i], rcvBuff, BUFF_SIZE, 0);
				if (ret <= 0) {
					FD_CLR(client[i], &initfds);
					closesocket(client[i]);
					client[i] = 0;
				}
				else if (ret == 0) {
					printf("Client disconnects\n");
					// set session here
					string fullAddr = getAddr(clientAddr);
					string res = getService(fullAddr, "BYE");
				}
				else if (ret > 0) {
					rcvBuff[ret] = 0;
					string str(rcvBuff);
					string fullAddr = getAddr(clientAddr);
					string res = getService(fullAddr, str);
					int n = res.length();
					//rcvBuff[n] = '\0';
					strcpy(rcvBuff, res.c_str());
					processData(rcvBuff, sendBuff);
					Send(client[i], sendBuff, n, 0);
				}
			}

			if (--nEvents <= 0)
				continue; //no more event
		}

	}

	closesocket(listenSock);
	WSACleanup();
	return 0;
}

sockaddr_in clientAddr;

/* The processData function copies the input string to output
* @param in Pointer to input string
* @param out Pointer to output string
* @return No return value
*/
void processData(char* in, char* out) {
	memcpy(out, in, BUFF_SIZE);
}

/* The recv() wrapper function */
int Receive(SOCKET s, char* buff, int size, int flags) {
	int n;
	printf("recv from\n");
	n = recv(s, buff, size, flags);
	if (n == SOCKET_ERROR) {
		printf("Error: %", WSAGetLastError());
	}

	return n;
}

/* The send() wrapper function*/
int Send(SOCKET s, char* buff, int size, int flags) {

	int n;
	n = send(s, buff, size, flags);
	if (n == SOCKET_ERROR)
		printf("Error: %", WSAGetLastError());

	return n;
}