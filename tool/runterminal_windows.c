#include <stdio.h>
#include <winsock2.h>

#include <tlspool/starttls.h>

starttls_t  *tlsdata_global = NULL;
uint32_t  startflags_global = 0;
const char  *localid_global = "";
const char *remoteid_global = "";

int         chanio_global;

DWORD WINAPI stdin_thread(LPVOID lpParam) {
	SOCKET     s = (SOCKET) lpParam;
	HANDLE     hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	DWORD      NumberOfBytesRead;
	CHAR       stdinbuf[8193];
	BOOL       bContinue = TRUE;
	
	while (bContinue) {
		BOOL fSuccess = ReadFile(
			hStdIn,             // stdin handle
			stdinbuf,           // buffer to receive reply
			sizeof (stdinbuf) - 1,  // size of buffer
			&NumberOfBytesRead, // number of bytes read
			NULL);              // overlapped
		bContinue = fSuccess;
		if (bContinue) {
			printf ("Read %ld bytes from stdin\n", NumberOfBytesRead);
			bContinue = NumberOfBytesRead > 0;
			if (bContinue) {
				stdinbuf[NumberOfBytesRead] = '\0';
				send (s, stdinbuf, NumberOfBytesRead, 0);
			}
		} else {
			fprintf (stderr, "Error reading stdin GLE=%d\n", GetLastError());
		}
	}
	fprintf (stderr, "Exiting thread\n");
	return 0;
}

void cancel_stdin_io(BOOL* fStdinDone, HANDLE hStdinThread) {
	if (!*fStdinDone) {
		*fStdinDone = TRUE;
		fprintf (stderr, "Cancelling stdin thread I/O\n");
		BOOL fSuccess = CancelSynchronousIo(hStdinThread);
		if (!fSuccess) {
			fprintf (stderr, "Error cancelling stdin thread I/O: GLE=%d\n", GetLastError());
		}
	}
}

void cancel_socket_io(BOOL fSocketDone, int chanio) {
	if (!fSocketDone) {
		fprintf (stderr, "Cancelling socket I/O\n");
		CancelIo((HANDLE) chanio);
	}
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
			fprintf (stderr, "%s\n", socketbuf);						
		}
	}
	return fSuccess;
}

BOOL readsocket(SOCKET s, LPWSABUF lpBuffers, LPWSAOVERLAPPED lpOverlapped, LPDWORD lpcbTransfer) {
	DWORD  flags = 0;
	BOOL   fSuccess;
	while (fSuccess = !WSARecv(s, lpBuffers, 1, NULL, &flags, lpOverlapped, NULL)) {
		fSuccess = overlapped_result(s, lpBuffers, lpOverlapped, lpcbTransfer);
	}
	return fSuccess;
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
	chanio_global     = chanio;

	CHAR          socketbuf[8192];
	WSAOVERLAPPED wsaOverlapped;
	WSABUF        wsaBuf;
	DWORD         flags = 0;
	DWORD         cbRead;
	DWORD         error;
	BOOL          fSuccess;
	BOOL          fSocketDone = FALSE;
	BOOL          fStdinDone = FALSE;
	wsaBuf.len = sizeof(socketbuf);
	wsaBuf.buf = socketbuf;

	ZeroMemory(&wsaOverlapped, sizeof(WSAOVERLAPPED));
	wsaOverlapped.hEvent = WSACreateEvent();
	fSuccess = readsocket((SOCKET) chanio, &wsaBuf, &wsaOverlapped, &cbRead);
	if (!fSuccess) {
		DWORD error = WSAGetLastError();
		if (error == WSA_IO_PENDING) {
			// Create the thread to begin execution on its own.
			DWORD dwThreadId;
			HANDLE hStdinThread = CreateThread( 
				NULL,                   // default security attributes
				0,                      // use default stack size  
				stdin_thread,           // thread function name
				(LPVOID) chanio,        // argument to thread function 
				0,                      // use default creation flags 
				&dwThreadId);   // returns the thread identifier 
			HANDLE hEvent[2];

			hEvent[0] = wsaOverlapped.hEvent;
			hEvent[1] = hStdinThread;
			fprintf (stderr, "Entering while loop\n");			
			while (!fSocketDone || !fStdinDone) {
				DWORD dwWaitObject = WSAWaitForMultipleEvents(
					2, // nCount
					hEvent, // lpHandles
					FALSE, // bWaitAll
					timeout == 0 ? WSA_INFINITE : timeout, // dwMilliseconds
					FALSE
				);
				fprintf(stderr, "dwWaitObject = %ld\n", dwWaitObject);
				if (dwWaitObject == WAIT_OBJECT_0) { // socket
					WSAResetEvent(wsaOverlapped.hEvent);
					fSuccess = overlapped_result((SOCKET) chanio, &wsaBuf, &wsaOverlapped, &cbRead);
					if (fSuccess) {
						if (cbRead == 0) { // socket closed
							cancel_stdin_io(&fStdinDone, hStdinThread);
							fSocketDone = TRUE;
						} else {
							fSuccess = readsocket((SOCKET) chanio, &wsaBuf, &wsaOverlapped, &cbRead);
							if (fSuccess || WSAGetLastError() != WSA_IO_PENDING) {
								cancel_stdin_io(&fStdinDone, hStdinThread);
								fSocketDone = TRUE;
							}
						}
					} else {
						fprintf (stderr, "Error reading socket GLE=%d\n", WSAGetLastError());
						cancel_stdin_io(&fStdinDone, hStdinThread);
						fSocketDone = TRUE;
					}
				} else if (dwWaitObject == WAIT_OBJECT_0 + 1) { // stdin thread terminated
					printf("stdin thread terminated\n");
					fStdinDone = TRUE;
					cancel_socket_io(fSocketDone, chanio);
				} else if (dwWaitObject == WSA_WAIT_TIMEOUT) {
					printf("timeout!\n");
					cancel_stdin_io(&fStdinDone, hStdinThread);
					cancel_socket_io(fSocketDone, chanio);
				} else if (dwWaitObject == WAIT_FAILED) {
					printf("WAIT FAILED: GLE=%ld", GetLastError());
					cancel_stdin_io(&fStdinDone, hStdinThread);
					cancel_socket_io(fSocketDone, chanio);
				}
			}
			fprintf (stderr, "After while loop\n");			
			CloseHandle(hStdinThread);
			hStdinThread = NULL;
		} else {
			fprintf (stderr, "Error reading socket GLE=%d\n", WSAGetLastError());
		}
	} else {
		fprintf (stderr, "Read %ld bytes from socket\n", cbRead);		
	}
}
