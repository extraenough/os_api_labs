#include "stdafx.h"
#include "win32client.h"
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <iostream>
#include <strsafe.h>

#define BUFSIZE 512
 
int _tmain(int argc, TCHAR *argv[]) 
{ 
	HANDLE hPipe; 
	LPTSTR lpvMessage=TEXT("Default message from client."); 
	TCHAR  chCurrent[BUFSIZE]; 
	TCHAR  chBuf[BUFSIZE]; 
	BOOL   fSuccess = FALSE; 
	DWORD  cbRead, cbToWrite, cbWritten, dwMode; 
	LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\mynamedpipe"); 

	TCHAR fHelp = TCHAR("-h");
	setlocale(LC_ALL, "Russian"); 

	/* обработка параметров */
	if(( argc == 1 ) || (argv[1] != &fHelp)){
		if(argv[1] == NULL){
			/* указание текущей директории программы в качестве параметра для сервера */
			GetCurrentDirectory(BUFSIZE, chCurrent);
			StringCchCat(chCurrent, BUFSIZE, TEXT("\\*"));
			lpvMessage = chCurrent;
		}else{
			/* указание входного параметра в качестве параметра для сервера */
			lpvMessage = argv[1];
		}

		while (1) 
		{ 
			/* подключение к pipe */
			hPipe = CreateFile( 
				lpszPipename,   // pipe name 
				GENERIC_READ |  // read and write access 
				GENERIC_WRITE, 
				0,              // no sharing 
				NULL,           // default security attributes
				OPEN_EXISTING,  // opens existing pipe 
				0,              // default attributes 
				NULL);          // no template file 
 
 
			if (hPipe != INVALID_HANDLE_VALUE) 
				break; 
 
 
			if (GetLastError() != ERROR_PIPE_BUSY) 
			{
				_tprintf( TEXT("Could not open pipe. GLE=%d\n"), GetLastError() ); 
				return -1;
			}
 
			if ( ! WaitNamedPipe(lpszPipename, 20000)) 
			{ 
				printf("Could not open pipe: 20 second wait timed out."); 
				return -1;
			} 
		}  
 
		/* изменение режима подключения на "чтение" */
		dwMode = PIPE_READMODE_MESSAGE; 
		fSuccess = SetNamedPipeHandleState( 
			hPipe,    // pipe handle 
			&dwMode,  // new pipe mode 
			NULL,     // don't set maximum bytes 
			NULL);    // don't set maximum time 
		if ( ! fSuccess) 
		{
			std::wcerr << "SetNamedPipeHandleState failed. GLE=" << GetLastError() << std::endl;
			//_tprintf( TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError() ); 
			return -1;
		}
 
		/* отправка параметров серверу */
 
		cbToWrite = (lstrlen(lpvMessage)+1)*sizeof(TCHAR);
		_tprintf( TEXT("Sending %d byte message: \"%s\"\n"), cbToWrite, lpvMessage); 

		fSuccess = WriteFile( 
			hPipe,                  // pipe handle 
			lpvMessage,             // message 
			cbToWrite,              // message length 
			&cbWritten,             // bytes written 
			NULL);                  // not overlapped 

		if ( ! fSuccess) 
		{
			std::wcerr << "WriteFile to pipe failed. GLE=" << GetLastError() << std::endl;
			//_tprintf( TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError() ); 
			return -1;
		}
		printf("\nMessage sent to server, receiving reply as follows:\n");
 
		/* получение ответа от сервера */
		bool end = false;
		do 
		{
			/* получение строки с информацией об элементе папки */
			fSuccess = ReadFile( 
				hPipe,    // pipe handle 
				chBuf,    // buffer to receive reply 
				BUFSIZE*sizeof(TCHAR),  // size of buffer 
				&cbRead,  // number of bytes read 
				NULL);    // not overlapped 
 
			if ( ! fSuccess && GetLastError() != ERROR_MORE_DATA )
				break; 
		
			/* обработка сообщения об окончании ответа от сервера */
			if(!wcsncmp(chBuf, L"end", 3)){
				end = true;
			}else{
				/* вывод строки на экран */
				_tprintf( TEXT("\"%s\"\n"), chBuf ); 
			}
		} while ( ! fSuccess || !end);  // цикл до получения сообщения об окончании

		if ( ! fSuccess)
		{
			std::wcerr << "ReadFile from pipe failed. GLE=" << GetLastError() << std::endl;
			//_tprintf( TEXT("ReadFile from pipe failed. GLE="), GetLastError() );
			return -1;
		}

		printf("\n<End of message, press ENTER to terminate connection and exit>\n");
		_getch();
 
		CloseHandle(hPipe); 		
	}
	else{
		/* обработка ввода ключа "-h" */
		std::cout << "lab 1. WinAPI."  << std::endl;
		_getch();
	}
 
	return 0; 
}
