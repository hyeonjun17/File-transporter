#include <stdio.h>
#include <stdlib.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <conio.h>
#pragma comment(lib, "ws2_32.lib")

#define PORT 60000
#define FILE_BUFSIZE 52428800 //50MB

char FILEBUF[FILE_BUFSIZE] = { 0 };

typedef struct FILE_DATA {
	char filename[FILENAME_MAX];
	ULONGLONG filesize;
}FILE_DATA, *PFILE_DATA;

int ChooseSide(void);
void BeServer(void);
void BeClient(void);

int main(void)
{
	while (1)
	{
		int side = ChooseSide();

		if (side == 3)
			break;

		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			printf("WSAStartup failed : %d\n", GetLastError());
			break;
		}

		if (side == 1)
		{
			BeServer();
			WSACleanup();
		}
		else if (side == 2)
		{
			BeClient();
			WSACleanup();
		}
		else
			continue;
	}

	printf("\n\n\nPress any key to exit..");
	_getch();

	return 0;
}

int ChooseSide(void)
{
	int num;
	printf("\n\n1. Be a server(receiver)\n2. Be a client(sender)\n3. Exit\n\nInput : ");
	scanf_s("%d", &num);
	getchar();
	return num;
}

void BeServer(void)
{
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		printf("socket error : %d\n", GetLastError());
		return;
	}

	SOCKADDR_IN addr;
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);
	addr.sin_family = AF_INET;

	if (bind(sock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		printf("bind error : %d\n", GetLastError());
		closesocket(sock);
		return;
	}

	system("C:\\Windows\\System32\\ipconfig");

	printf("\n\nWaiting For Client to Connect\n\n");

	if (listen(sock, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("listen error : %d\n", GetLastError());
		closesocket(sock);
		return;
	}

	SOCKET clnt_sock = accept(sock, NULL, NULL);
	if (clnt_sock == INVALID_SOCKET)
	{
		printf("accept error : %d\n", WSAGetLastError());
		return;
	}
	printf("\n\nClient Connected!\n\n");
	Sleep(3000);
	//receive file data
	printf("\n\nReceiving File Name and Size..\n\n");
	FILE_DATA fdata = { 0 };
	int result = 0;
	unsigned int received = 0;
	while (result = recv(clnt_sock, &fdata, sizeof(fdata), 0) > 0)
	{
		printf("Bytes received : %d\n", result);
		received += result;
		if (received >= sizeof(fdata))
			break;
	}
	if(result < 0)
	{
		printf("recv error : %d\n", WSAGetLastError());
		closesocket(sock);
		closesocket(clnt_sock);
		return;
	}
	printf("\n\nTotal Received : %u\n\n", received);
	printf("\n\nFile Name : %s\n\n", fdata.filename);
	printf("\n\nFile Size : %llu\n\n", fdata.filesize);
	//create file
	printf("\n\nCreating File..\n\n");
	HANDLE hFile = CreateFileA(fdata.filename,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("CreateFileA error : %d\n", GetLastError());
		return;
	}
	//receive file
	printf("\n\nReceiving File..\n\n");
	DWORD dwNumOfBytesWritten = 0;
	ULONGLONG ulReceived_Size = 0;
	result = 0;
	do {
		if (ulReceived_Size >= fdata.filesize)
		{
			printf("\n\nTotal Received Bytes : %llu\n\n", ulReceived_Size);
			break;
		}
		ZeroMemory(FILEBUF, FILE_BUFSIZE);
		result = recv(clnt_sock, FILEBUF, FILE_BUFSIZE, 0);
		if (result > 0)
			printf("Bytes received : %d\n", result);
		else if (result < 0)
		{
			printf("recv error : %d\n", WSAGetLastError());
			closesocket(sock);
			closesocket(clnt_sock);
			return;
		}
		ulReceived_Size += result;
		BOOL boolean = WriteFile(hFile, FILEBUF, result,
			&dwNumOfBytesWritten, NULL);
		if (boolean && dwNumOfBytesWritten == result)
			printf("\n\nWriting File..\n\n");
		else
		{
			printf("writefile error : %d\n", GetLastError());
			break;
		}
	} while (1);
	//shutdown receiving
	shutdown(clnt_sock, SD_RECEIVE);
	//send check value
	char check = FALSE;
	if (fdata.filesize == ulReceived_Size)
		check = TRUE;
	unsigned int sent = 0;
	result = 0;
	while (result = send(clnt_sock, &check, sizeof(check), 0) > 0)
	{
		printf("Bytes received : %d\n", result);
		sent += result;
		if (sent >= sizeof(check))
			break;
	}
	if (result < 0)
	{
		printf("send error : %d\n", WSAGetLastError());
		closesocket(sock);
		closesocket(clnt_sock);
		return;
	}

	closesocket(sock);
	closesocket(clnt_sock);

	if (fdata.filesize == ulReceived_Size)
		printf("\n\nFile Received Successfully!\n\n");
	else
		printf("\n\nCouldn't Receive File Successfully..\n\n");
	CloseHandle(hFile);
}

void BeClient(void)
{
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		printf("socket error : %d\n", GetLastError());
		return;
	}

	char ip[16] = { 0 };
	printf("\nEnter ip address of server : ");
	scanf_s("%s", ip, sizeof(ip));
	getchar();
	putchar('\n');

	

	SOCKADDR_IN addr;
	ZeroMemory(&addr, sizeof(addr));
	inet_pton(AF_INET, ip, &addr.sin_addr.S_un.S_addr);
	addr.sin_port = htons(PORT);
	addr.sin_family = AF_INET;

	if (connect(sock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		printf("connect error : %d\n", WSAGetLastError());
		closesocket(sock);
		return;
	}

	printf("\nconnected to server!\n\n");
	FILE_DATA fdata = { 0 };
	//get file name
	ZeroMemory(fdata.filename, sizeof(fdata.filename));
	printf("Enter file name : ");
	scanf_s("%s", fdata.filename, sizeof(fdata.filename));
	getchar();
	//open file
	HANDLE hFile = CreateFileA(fdata.filename, GENERIC_READ,
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("createfilea error : %d\n", GetLastError());
		closesocket(sock);
		return;
	}
	//get file size
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if (!GetFileAttributesExA(fdata.filename, GetFileExInfoStandard, &fad))
	{
		printf("getfileattributeexa error : %d\n", GetLastError());
		closesocket(sock);
		return;
	}
	LARGE_INTEGER liSize;
	ZeroMemory(&liSize, sizeof(liSize));
	liSize.HighPart = fad.nFileSizeHigh;
	liSize.LowPart = fad.nFileSizeLow;
	fdata.filesize = liSize.QuadPart;
	//send file name and size
	printf("\n\nSending File Name and Size..(%s, %llu)\n\n", fdata.filename, fdata.filesize);
	int result = 0;
	unsigned int sent = 0;
	while (result = send(sock, &fdata, sizeof(fdata), 0) > 0)
	{
		printf("Bytes sent : %d\n", result);
		sent += result;
		if (sent >= sizeof(fdata))
			break;
	}
	if (result < 0)
	{
		printf("send error : %d\n", WSAGetLastError());
		closesocket(sock);
		return;
	}
	//send file
	printf("\n\nSending File..\n\n");
	DWORD dwBytesRead;
	result = 0;
	sent = 0;
	do {
		//read file
		ZeroMemory(FILEBUF, FILE_BUFSIZE);
		if (!ReadFile(hFile, FILEBUF, FILE_BUFSIZE, &dwBytesRead, NULL))
		{
			printf("readfile error : %d\n", GetLastError());
			closesocket(sock);
			return;
		}
		result = send(sock, FILEBUF, dwBytesRead, 0);
		if (result > 0)
			printf("Bytes sent : %d\n", result);
		else if (result < 0)
		{
			printf("send error : %d\n", WSAGetLastError());
			closesocket(sock);
			return;
		}
		sent += result;
		if (sent >= fdata.filesize)
		{
			printf("\n\nTotal Sent Bytes : %lu\n\n", sent);
			break;
		}
	} while (1);
	//shutdown sending
	shutdown(sock, SD_SEND);
	//check if server got file
	char check = FALSE;
	unsigned int received = 0;
	result = 0;
	while (result = recv(sock, &check, sizeof(check), 0) > 0)
	{
		printf("Bytes received : %d\n", result);
		received += result;
		if (received >= sizeof(check))
			break;
	}
	if (result < 0)
	{
		printf("receive error : %d\n", WSAGetLastError());
		closesocket(sock);
		return;
	}
	if(check)
		printf("\n\nSent File Successfully!\n\n");
	else
		printf("\n\nSomething Went Wrong\n\n");
	closesocket(sock);
	CloseHandle(hFile);
}