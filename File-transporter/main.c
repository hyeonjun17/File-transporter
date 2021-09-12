#include <stdio.h>
#include <Windows.h>
#include <WinSock2.h>
#include <conio.h>

#define PORT 60000
#define NAME_BUFSIZE sizeof(char) * 50
#define FILE_BUFSIZE 52428800 //50MB
#define LENGTH_BUFSIZE sizeof(unsigned __int64) //8byte

char NAMEBUF[NAME_BUFSIZE] = { 0 };
char FILEBUF[FILE_BUFSIZE] = { 0 };

int ChooseSide(void)
{
	int num;
	printf("1. Be a server(receiver)\n2. Be a client(sender)\n3. Exit\n\nInput : ");
	scanf_s("%d", &num);
	return num;
}

void BeServer(void)
{
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		printf("socket error : %d", GetLastError());
		return;
	}

	SOCKADDR_IN addr;
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	addr.sin_port = PORT;
	addr.sin_family = AF_INET;

	if (bind(sock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		printf("bind error : %d", GetLastError());
		closesocket(sock);
		return;
	}

	if (listen(sock, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("listen error : %d", GetLastError());
		closesocket(sock);
		return;
	}
	//receive file name
	int result = 0;
	while (result = recv(sock, NAMEBUF, NAME_BUFSIZE, 0) > 0)
		if (result >= NAME_BUFSIZE)
			break;
	if (result == SOCKET_ERROR)
	{
		printf("recv error : %d", WSAGetLastError());
		closesocket(sock);
		return;
	}
	//receive file length
	unsigned __int64 length = 0;
	result = 0;
	while (result = recv(sock, &length, LENGTH_BUFSIZE, 0) > 0)
		if (result >= LENGTH_BUFSIZE)
			break;
	if (result == SOCKET_ERROR)
	{
		printf("recv error : %d", WSAGetLastError());
		closesocket(sock);
		return;
	}
	//receive file
	result = 0;
	while (result = recv(sock, FILEBUF, FILE_BUFSIZE, 0) > 0)
		if (result >= FILE_BUFSIZE)
			break;
	if (result == SOCKET_ERROR)
	{
		printf("recv error : %d", WSAGetLastError());
		closesocket(sock);
		return;
	}

	closesocket(sock);

	HANDLE hFile = CreateFileA(NAMEBUF,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("CreateFileA error : %d", GetLastError());
		return;
	}

	BOOL boolean;
	int NumOfBytesWritten = 0;
	boolean = WriteFile(hFile, FILEBUF, FILE_BUFSIZE,
		&NumOfBytesWritten, NULL);
	if (boolean == FALSE)
	{
		printf("WriteFileA failed\n");
	}

	CloseHandle(hFile);
}

void BeClient(void)
{

}

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