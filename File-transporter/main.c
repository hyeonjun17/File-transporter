#include <stdio.h>
#include <Windows.h>
#include <WinSock2.h>
#include <conio.h>

#define PORT 60000
#define NAME_BUFSIZE FILENAME_MAX
#define FILE_BUFSIZE 52428800 //50MB
#define FILESIZE_LENGTH sizeof(unsigned __int64) //8byte

char NAMEBUF[NAME_BUFSIZE] = { 0 };
char FILEBUF[FILE_BUFSIZE] = { 0 };

int ChooseSide(void)
{
	int num;
	printf("1. Be a server(receiver)\n2. Be a client(sender)\n3. Exit\n\nInput : ");
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

	char name[50] = { 0 };
	if (gethostname(name, sizeof(char) * 50) == SOCKET_ERROR)
	{
		printf("gethostname error : %d\n", WSAGetLastError());
	}
	else
	{
		HOSTENT* hst = gethostbyname(name);
		if (hst == NULL)
		{
			printf("gethostbyname error : %d\n", WSAGetLastError());
		}
		else
		{
			int i = 0;
			IN_ADDR addr;
			while (hst->h_addr_list[i] != 0)
			{
				addr.S_un.S_addr = 0;
				addr.S_un.S_addr = *(u_long*)hst->h_addr_list[i];
				printf("IP address #%d: %s\n", i, inet_ntoa(addr));
				i++;
			}
		}
	}

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
	printf("\n\nWait..\n\n");
	Sleep(3000);
	//receive file name
	int result = 0;
	do {
		result = recv(clnt_sock, NAMEBUF, NAME_BUFSIZE, 0);
		if (result > 0)
			printf("Bytes received : %d\n", result);
		else if (result == 0)
			printf("Connection closed\n");
		else
		{
			printf("recv error : %d", WSAGetLastError());
			closesocket(sock);
			closesocket(clnt_sock);
			return;
		}
	} while (result > 0);
	//receive file size
	unsigned __int64 _64FileSize = 0;
	result = 0;
	do {
		result = recv(clnt_sock, &_64FileSize, FILESIZE_LENGTH, 0);
		if (result > 0)
			printf("Bytes received : %d\n", result);
		else if (result == 0)
			printf("Connection closed\n");
		else
		{
			printf("recv error : %d", WSAGetLastError());
			closesocket(sock);
			closesocket(clnt_sock);
			return;
		}
	} while (result > 0);
	//receive file
	unsigned __int64 _64Received_Size = 0;
	result = 0;
	do {
		result = recv(clnt_sock, FILEBUF, FILE_BUFSIZE, 0);
		if (result > 0)
			printf("Bytes received : %d\n", result);
		else if (result == 0)
			printf("Connection closed\n");
		else
		{
			printf("recv error : %d", WSAGetLastError());
			closesocket(sock);
			closesocket(clnt_sock);
			return;
		}
		_64Received_Size += result;
	} while (result > 0 && _64Received_Size < _64FileSize);

	closesocket(sock);
	closesocket(clnt_sock);

	HANDLE hFile = CreateFileA(NAMEBUF,
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

	int NumOfBytesWritten = 0;
	BOOL boolean = WriteFile(hFile, FILEBUF, FILE_BUFSIZE,
		&NumOfBytesWritten, NULL);
	if (boolean == FALSE)
		printf("WriteFileA error : %d\n", GetLastError());
	else
		printf("\n\nFile Received Successfully!\n");
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

	char IP[16] = { 0 };
	printf("\nEnter ip address of server : ");
	scanf_s("%s", IP, sizeof(IP));
	getchar();
	putchar('\n');

	SOCKADDR_IN addr;
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_addr.S_un.S_addr = inet_addr(IP);
	addr.sin_port = htons(PORT);
	addr.sin_family = AF_INET;

	if (connect(sock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		printf("connect error : %d\n", WSAGetLastError());
		closesocket(sock);
		return;
	}

	printf("\nconnected to server!\n\n");
	//get file name
	ZeroMemory(NAMEBUF, sizeof(NAMEBUF));
	printf("Enter file name : ");
	scanf_s("%s", NAMEBUF, sizeof(NAMEBUF));
	getchar();
	//get file size
	unsigned __int64 _64FileSize = 0;
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if (!GetFileAttributesExA(NAMEBUF, GetFileExInfoStandard, &fad))
	{
		printf("getfileattributeexa error : %d", GetLastError());
		closesocket(sock);
		return;
	}
	LARGE_INTEGER liSize;
	ZeroMemory(&liSize, sizeof(liSize));
	liSize.HighPart = fad.nFileSizeHigh;
	liSize.LowPart = fad.nFileSizeLow;
	_64FileSize = liSize.QuadPart;
	//read file
	HANDLE hFile = CreateFileA(NAMEBUF, GENERIC_READ,
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("createfilea error : %d", GetLastError());
		closesocket(sock);
		return;
	}
	DWORD dwBytesRead;
	if (ReadFile(hFile, FILEBUF, _64FileSize, &dwBytesRead, NULL) == FALSE)
	{
		printf("readfile error : %d", GetLastError());
		closesocket(sock);
		return;
	}
	//send file name
	int result = 0;
	do {
		result = send(sock, NAMEBUF, sizeof(NAMEBUF), 0);
		if (result > 0)
			printf("\nBytes sent : %d", result);
		else if (result == 0)
			printf("\nConnection closed");
		else
		{
			printf("send error : %d", WSAGetLastError());
			closesocket(sock);
			return;
		}
	} while (result > 0);
	//send file size
	result = 0;
	do {
		result = send(sock, _64FileSize, sizeof(_64FileSize), 0);
		if (result > 0)
			printf("\nBytes sent : %d", result);
		else if (result == 0)
			printf("\nConnection closed");
		else
		{
			printf("send error : %d", WSAGetLastError());
			closesocket(sock);
			return;
		}
	} while (result > 0);
	//send file
	unsigned __int64 _64SentFile = 0;
	result = 0;
	do {
		result = send(sock, FILEBUF, sizeof(FILEBUF), 0);
		if (result > 0)
			printf("\nBytes sent : %d", result);
		else if (result == 0)
			printf("\nConnection closed");
		else
		{
			printf("send error : %d", WSAGetLastError());
			closesocket(sock);
			return;
		}
		_64SentFile += result;
	} while (result > 0 && _64SentFile < _64FileSize);
	printf("\n\nSent File Successfully!\n\n");
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