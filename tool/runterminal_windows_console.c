#include <stdio.h>
#include <winsock2.h>
#include <tlspool/starttls.h>
 
starttls_t  *tlsdata_global    = NULL;
uint32_t     startflags_global = 0;
const char  *localid_global    = "";
const char  *remoteid_global   = "";
 
void cancel_socket_io(int chanio) {
	fprintf (stderr, "Cancelling socket I/O\n");
	CancelIo((HANDLE) chanio);
}

BOOL overlapped_result(SOCKET s, LPWSABUF lpBuffers, LPWSAOVERLAPPED lpWsaOverlapped, LPDWORD lpcbTransfer) {
	DWORD         cbRead;
	
	DWORD      dwFlags = 0;
	BOOL fSuccess = WSAGetOverlappedResult(s, lpWsaOverlapped, lpcbTransfer, TRUE, &dwFlags);
	if (fSuccess) {
		DWORD cbRead = *lpcbTransfer;
		PCHAR socketbuf = lpBuffers->buf;
		
		fprintf (stderr, "Read %ld bytes from socket\n", cbRead);
		WSAResetEvent(lpWsaOverlapped->hEvent);
		socketbuf[cbRead] = '\0';
		if (cbRead > 0) {
			fprintf (stderr, "%s", socketbuf);						
 		}
	} else {
 		fprintf (stderr, "Error in WSAGetOverlappedResult GLE=%d\n", GetLastError());
 	}
 	return fSuccess;
}

BOOL readsocket(SOCKET s, LPWSABUF lpBuffers, LPWSAOVERLAPPED lpOverlapped, LPDWORD lpcbTransfer) {
	DWORD  flags = 0;
	BOOL   fSuccess;
	while (fSuccess = !WSARecv(s, lpBuffers, 1, NULL, &flags, lpOverlapped, NULL)) {
		fprintf(stderr, "readsocket: fSuccess = %d", fSuccess);
		fSuccess = overlapped_result(s, lpBuffers, lpOverlapped, lpcbTransfer);
	}
	return fSuccess;
}

static CHAR       stdinbuf[8192];

int process_console_io(HANDLE hStdIn) {
	INPUT_RECORD      irInBuf[128]; 
	KEY_EVENT_RECORD *ker;
	DWORD             NumberOfRecordsRead;
	static int        idx = 0;
	int               rc = 0;
	
	if (ReadConsoleInput( 
		hStdIn,      // input buffer handle 
		irInBuf,     // buffer to read into 
		128,         // size of read buffer 
		&NumberOfRecordsRead))  // number of records read
	{
		int i;
		for (i = 0; i < NumberOfRecordsRead; i++) 
		{
			switch (irInBuf[i].EventType) 
			{ 
				case KEY_EVENT: // keyboard input 
					ker = &irInBuf[i].Event.KeyEvent; 
					if (ker->bKeyDown) {
						CHAR ch = ker->uChar.AsciiChar;
						if (ch != 0 && idx < sizeof(stdinbuf) - 2) {
							if (ch < ' ') {
								if (ch == '\r') { // Carriage Return, return # bytes in buffer
									stdinbuf[idx++] = '\r';
									stdinbuf[idx++] = '\n';
									printf("\n");
									rc = idx;
									idx = 0;
								} else if (ch == '\b') {  // Backspace
									if (idx > 0) {
										printf("\b \b");
										idx--;
									}
								} else if (ch == 4 && idx == 0) { // Ctrl-D
									rc = -1;
								}
							} else {
								printf("%c", ch);
								stdinbuf[idx++] = ch;
							}
						}
					}
					break; 
 
				case MOUSE_EVENT: // mouse input 
					break; 
 
				case WINDOW_BUFFER_SIZE_EVENT: // scrn buf. resizing 
					break; 
 
				case FOCUS_EVENT:  // disregard focus events 
 
				case MENU_EVENT:   // disregard menu events 
					break; 
 
				default: 
					break; 
			} 
		}
	} else {
		fprintf (stderr, "Error in ReadConsoleInput GLE=%d\n", GetLastError());
		rc = -2;
	}
	return rc;
}

int runterminal (int chanio, int *sigcont, starttls_t *tlsdata,
                  uint32_t startflags, const char *localid, const char *remoteid, int timeout) {

	int rc = 0;
	fprintf(stderr, "chanio: %d\n", chanio);
	//
	// Make parameters tlsdata globally known (imperfect solution)
	tlsdata_global    = tlsdata;
	startflags_global = startflags;
	localid_global    = localid;
	remoteid_global   = remoteid;

	CHAR          socketbuf[8192];
	WSAOVERLAPPED wsaOverlapped;
	WSABUF        wsaBuf;
	DWORD         flags = 0;
	DWORD         cbRead;
	DWORD         error;
	BOOL          fSuccess;
	wsaBuf.len = sizeof(socketbuf);
	wsaBuf.buf = socketbuf;

	ZeroMemory(&wsaOverlapped, sizeof(WSAOVERLAPPED));
	wsaOverlapped.hEvent = WSACreateEvent();
	fSuccess = readsocket((SOCKET) chanio, &wsaBuf, &wsaOverlapped, &cbRead);
	if (!fSuccess) {
		DWORD error = WSAGetLastError();
		if (error == WSA_IO_PENDING) {
			HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
			DWORD FileType = GetFileType(hStdIn);
			if (FileType == FILE_TYPE_CHAR) {
				BOOL   fSocketDone = FALSE;
				HANDLE hEvent[2];
				
				hEvent[0] = wsaOverlapped.hEvent;
				hEvent[1] = hStdIn;
				fprintf (stderr, "Entering while loop\n");
				int idx = 0;
				while (!fSocketDone) {
					DWORD dwWaitObject = WSAWaitForMultipleEvents(
						2, // nCount
						hEvent, // lpHandles
						FALSE, // bWaitAll
						timeout == 0 ? WSA_INFINITE : timeout, // dwMilliseconds
						FALSE
					);
					//fprintf(stderr, "dwWaitObject = %ld\n", dwWaitObject);
					if (dwWaitObject == WAIT_OBJECT_0) { // socket
						WSAResetEvent(wsaOverlapped.hEvent);
						fSuccess = overlapped_result((SOCKET) chanio, &wsaBuf, &wsaOverlapped, &cbRead);
						if (fSuccess) {
							if (cbRead == 0) { // socket closed 								
								fSocketDone = TRUE;
							} else {
								fSuccess = readsocket((SOCKET) chanio, &wsaBuf, &wsaOverlapped, &cbRead);
								if (fSuccess || WSAGetLastError() != WSA_IO_PENDING) {
									fSocketDone = TRUE;
								}
 							}
						} else {
							fprintf (stderr, "Error reading socket GLE=%d\n", WSAGetLastError());
							fSocketDone = TRUE;
						}
					} else if (dwWaitObject == WAIT_OBJECT_0 + 1) { // hStdIn console
						int rc = process_console_io(hStdIn);
						if (rc > 0) {
							printf ("Read %ld bytes from stdin\n", rc);
							send ((SOCKET) chanio, stdinbuf, rc, 0);
						} else if (rc < 0) {
							cancel_socket_io((SOCKET) chanio);
					} else if (dwWaitObject == WSA_WAIT_TIMEOUT) {
						printf("timeout!\n");
						cancel_socket_io(chanio);
					} else if (dwWaitObject == WAIT_FAILED) {
						printf("WAIT FAILED: GLE=%ld", GetLastError());
						cancel_socket_io(chanio); 						}
					}
				}
				fprintf (stderr, "After while loop\n");			
			} else {
				fprintf (stderr, "Console only\n");
			}
		} else {
			fprintf (stderr, "Error reading socket GLE=%d\n", WSAGetLastError());
		}
	} else {
		fprintf (stderr, "Read %ld bytes from socket\n", cbRead);		
	}
	CloseHandle(wsaOverlapped.hEvent);
}
