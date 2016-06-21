#include "stdafx.h"
#include "win32server.h"
#include <windows.h> 
#include <stdio.h> 
#include <conio.h>
#include <iostream>
#include <iomanip>
#include <tchar.h>
#include <strsafe.h>

#define BUFSIZE 512
 
DWORD WINAPI InstanceThread(LPVOID); 
BOOL GetLastWriteTime(HANDLE hFile, LPTSTR lpszString, DWORD dwSize);


int _tmain(VOID) 
{ 
	BOOL   fConnected = FALSE; 
	DWORD  dwThreadId = 0; 
	HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = NULL; 
	LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\mynamedpipe"); 
	DWORD fSize = 0;
 
	
	/* обработка подключения клиентов */
 
	for (;;) 
	{ 

		/* создание named-pipe */
		_tprintf( TEXT("\nPipe Server: Main thread awaiting client connection on %s\n"), lpszPipename);
		hPipe = CreateNamedPipe( 
			lpszPipename,             // pipe name 
			PIPE_ACCESS_DUPLEX,       // read/write access 
			PIPE_TYPE_MESSAGE |       // message type pipe 
			PIPE_READMODE_MESSAGE |   // message-read mode 
			PIPE_WAIT,                // blocking mode 
			PIPE_UNLIMITED_INSTANCES, // max. instances  
			BUFSIZE,                  // output buffer size 
			BUFSIZE,                  // input buffer size 
			0,                        // client time-out 
			NULL);                    // default security attribute 

		if (hPipe == INVALID_HANDLE_VALUE) 
		{
			_tprintf(TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError()); 
			return -1;
		}
 
		/* подключение сервера к pipe */ 
 
		fConnected = ConnectNamedPipe(hPipe, NULL) ? 
		TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 
 
		if (fConnected) 
		{ 
			printf("Client connected, creating a processing thread.\n"); 
      
			// Create a thread for this client. 
			hThread = CreateThread( 
				NULL,              // no security attribute 
				0,                 // default stack size 
				InstanceThread,    // thread proc
				(LPVOID) hPipe,    // thread parameter 
				0,                 // not suspended 
				&dwThreadId);      // returns thread ID 

			if (hThread == NULL) 
			{
				_tprintf(TEXT("CreateThread failed, GLE=%d.\n"), GetLastError()); 
				return -1;
			}
			else CloseHandle(hThread); 
		} 
		else 
		// The client could not connect, so close the pipe. 
		CloseHandle(hPipe); 
	} 
	
	return 0; 
}
 
DWORD WINAPI InstanceThread(LPVOID lpvParam)
// This routine is a thread processing function to read from and reply to a client
// via the open pipe connection passed from the main loop. Note this allows
// the main loop to continue executing, potentially creating more threads of
// of this procedure to run concurrently, depending on the number of incoming
// client connections.
{ 
	HANDLE hHeap      = GetProcessHeap();
	TCHAR* pchRequest = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE*sizeof(TCHAR));
	TCHAR* pchReply   = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE*sizeof(TCHAR));

	DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0; 
	BOOL fSuccess = FALSE;
	HANDLE hPipe  = NULL;

	// Do some extra error checking since the app will keep running even if this
	// thread fails.

	if (lpvParam == NULL)
	{
		printf( "\nERROR - Pipe Server Failure:\n");
		printf( "   InstanceThread got an unexpected NULL value in lpvParam.\n");
		printf( "   InstanceThread exitting.\n");
		if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
		if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
		return (DWORD)-1;
	}

	if (pchRequest == NULL)
	{
		printf( "\nERROR - Pipe Server Failure:\n");
		printf( "   InstanceThread got an unexpected NULL heap allocation.\n");
		printf( "   InstanceThread exitting.\n");
		if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
		return (DWORD)-1;
	}

	if (pchReply == NULL)
	{
		printf( "\nERROR - Pipe Server Failure:\n");
		printf( "   InstanceThread got an unexpected NULL heap allocation.\n");
		printf( "   InstanceThread exitting.\n");
		if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
		return (DWORD)-1;
	}

	// Print verbose messages. In production code, this should be for debugging only.
	printf("InstanceThread created, receiving and processing messages.\n");

	// The thread's parameter is a handle to a pipe object instance. 
 
	hPipe = (HANDLE) lpvParam; 

	// Loop until done reading
	while (1) 
	{ 
		/* получение от клиента сообщения (путь к папке) */
		fSuccess = ReadFile( 
		hPipe,        // handle to pipe 
		pchRequest,    // buffer to receive data 
		BUFSIZE*sizeof(TCHAR), // size of buffer 
		&cbBytesRead, // number of bytes read 
		NULL);        // not overlapped I/O 

		if (!fSuccess || cbBytesRead == 0)
		{   
			if (GetLastError() == ERROR_BROKEN_PIPE)
			{
				_tprintf(TEXT("InstanceThread: client disconnected.\n"), GetLastError()); 
			}
			else
			{
				_tprintf(TEXT("InstanceThread ReadFile failed, GLE=%d.\n"), GetLastError()); 
			}
			break;
		}


		wchar_t const *pSearchMask = pchRequest;

		WIN32_FIND_DATAW wfd;
		/* получение первого элемента папки*/
		HANDLE const hFind = FindFirstFile(pSearchMask, &wfd);
 
		if (INVALID_HANDLE_VALUE != hFind)
		{
			do
			{
				double filesize = 0;

				/* временные буферы для хранения информации о текущем элементе папки */
				TCHAR bufMain[BUFSIZE];
				TCHAR buf[32];
				TCHAR bufTemp[32];
				/* определение: файл или папка */
				//std::wcout << std::setw(4);
				if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
					StringCchPrintf(bufMain, BUFSIZE, TEXT("dir "));
					//StringCchCat(bufMain, 0, TEXT("dir "));
				}else
					StringCchPrintf(bufMain, BUFSIZE, TEXT("    "));
					//std::wcout << "";*/
				
				/* получение атрибутов файла */
				if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
					
					/* получение размера файла */
					filesize = (wfd.nFileSizeLow - wfd.nFileSizeHigh);				
					int count = 0;
					while(filesize > 512){
						filesize /= 1024;
						count += 1;
					}
					StringCchPrintf(buf, 32, TEXT(""));
					StringCchPrintf(bufTemp, 32, TEXT(""));

					if(count == 0){
						StringCchPrintf(bufTemp, 32, TEXT("%3.2lf B "), filesize);
					}else{
						if(count == 1){
							StringCchPrintf(bufTemp, 32, TEXT("%3.2lfKB "), filesize);
						}else{
							if(count == 2){
								StringCchPrintf(bufTemp, 32, TEXT("%3.2lfMB "), filesize);
							}else{
								StringCchPrintf(bufTemp, 32, TEXT("%3.2lfGB "), filesize);
							}
						}
					}
					//std::wcout << std::setw(10) << bufDate ;
					if(filesize < 100){
						if(filesize < 10)
							StringCchPrintf(buf, 32, TEXT("  %s "), bufTemp);
						else
							StringCchPrintf(buf, 32, TEXT(" %s "), bufTemp);
					}
					//StringCchPrintf(bufDate, 32, bufDateTemp);
					StringCchCat(bufMain, BUFSIZE, buf);//bufMain = bufMain + bufDate;
					
					/* получение даты последнего изменения файла*/
					SYSTEMTIME stUTC, stLocal;
					FileTimeToSystemTime(&wfd.ftLastWriteTime, &stUTC);
					SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);				
					//TCHAR sDate[64];
					StringCchPrintf(buf, 32, 
						TEXT("%02d.%02d.%d %02d:%02d "),
						stLocal.wDay, stLocal.wMonth, stLocal.wYear,
						stLocal.wHour, stLocal.wMinute);
					StringCchCat(bufMain, BUFSIZE, buf);
				}else{
					StringCchCat(bufMain, BUFSIZE, TEXT("                           "));
				}
				
				/* запись имени файла во временный буфер */
				StringCchCat(bufMain, BUFSIZE, &wfd.cFileName[0]);

				lstrcpyW(pchReply, bufMain);
				cbReplyBytes = (lstrlen(pchReply)+1)*sizeof(TCHAR);
				
				/* отправка сообщения в pipe (строка с информацией об элементе папки) */
				fSuccess = WriteFile( 
					hPipe,        // handle to pipe 
					pchReply,      // buffer to write from 
					cbReplyBytes, // number of bytes to write 
					&cbWritten,   // number of bytes written 
					NULL);        // not overlapped I/O 

				if (!fSuccess || cbReplyBytes != cbWritten)
				{   
					std::wcerr << "InstanceThread WriteFile failed. GLE=" << GetLastError() << std::endl;
					//_tprintf(TEXT("InstanceThread WriteFile failed, GLE=%d.\n"), GetLastError()); 
					break;
				}

			} while (FALSE != FindNextFile(hFind, &wfd));
		
			FindClose(hFind);
		}

		
		/* отправка сообщения о завершении ответа сервера */
		fSuccess = WriteFile( 
			hPipe,        // handle to pipe 
			TEXT("end"),     // buffer to write from 
			(lstrlen(TEXT("end"))+1)*sizeof(TCHAR), // number of bytes to write 
			&cbWritten,   // number of bytes written 
			NULL);        // not overlapped I/O 

		if (!fSuccess || cbReplyBytes != cbWritten)
		{   
			_tprintf(TEXT("InstanceThread WriteFile failed, GLE=%d.\n"), GetLastError()); 
			break;
		}
	}

	
	/* очистка выделенной памяти и закрытие соединения с клиентом */
 
	FlushFileBuffers(hPipe); 
	DisconnectNamedPipe(hPipe); 
	CloseHandle(hPipe); 

	HeapFree(hHeap, 0, pchRequest);
	HeapFree(hHeap, 0, pchReply);

	printf("InstanceThread exitting.\n");
	return 1;
}

