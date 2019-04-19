#include "whoami.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>

#include <syslog.h>
#include <fcntl.h>

#include <tlspool/commands.h>
#include <tlspool/internal.h>

#include <winsock2.h>
#include <windows.h>
#ifndef __MINGW64__
#define WEOF ((wint_t)(0xFFFF))
#endif

#define PIPE_TIMEOUT 5000
#define BUFSIZE 4096

#define CONNECTING_STATE 0
#define READING_STATE 1
#define INSTANCES 4
#define PIPE_TIMEOUT 5000
#define BUFSIZE 4096

static VOID DisconnectAndReconnect(DWORD);
static BOOL ConnectToNewClient(HANDLE, LPOVERLAPPED);

PIPEINST Pipe[INSTANCES];
HANDLE hEvents[INSTANCES];
extern char szPipename[1024];

static int convert_socket_to_posix(SOCKET s) {
	int rc;
	
	if (s == INVALID_SOCKET) {
		rc = -1;
	} else if (s <= INT_MAX) {
		rc = (int) s;
	} else {
		closesocket(s);			
		rc = -1;
	}
	return rc;
}

static int socket_from_protocol_info (LPWSAPROTOCOL_INFOW lpProtocolInfo)
{
	return convert_socket_to_posix(WSASocketW(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, lpProtocolInfo, 0, 0));
}

// ConnectToNewClient(HANDLE, LPOVERLAPPED)
// This function is called to start an overlapped connect operation.
// It returns TRUE if an operation is pending or FALSE if the
// connection has been completed.

static BOOL ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo)
{
   BOOL fConnected, fPendingIO = FALSE;

// Start an overlapped connection for this pipe instance.
   fConnected = ConnectNamedPipe(hPipe, lpo);

// Overlapped ConnectNamedPipe should return zero.
   if (fConnected)
   {
      tlog (TLOG_UNIXSOCK, LOG_CRIT, "ConnectNamedPipe failed with %d.\n", GetLastError());
      return 0;
   }

   switch (GetLastError())
   {
   // The overlapped connection in progress.
      case ERROR_IO_PENDING:
         fPendingIO = TRUE;
         break;

   // Client is already connected, so signal an event.

      case ERROR_PIPE_CONNECTED:
         if (SetEvent(lpo->hEvent))
            break;

   // If an error occurs during the connect operation...
      default:
      {
         tlog (TLOG_UNIXSOCK, LOG_CRIT, "ConnectNamedPipe failed with %d.\n", GetLastError());
         return 0;
      }
   }
   return fPendingIO;
}

// DisconnectAndReconnect(DWORD)
// This function is called when an error occurs or when the client
// closes its handle to the pipe. Disconnect from this client, then
// call ConnectNamedPipe to wait for another client to connect.

static VOID DisconnectAndReconnect(DWORD i)
{
// Disconnect the pipe instance.

   if (! DisconnectNamedPipe(Pipe[i].hPipeInst) )
   {
      tlog (TLOG_UNIXSOCK, LOG_CRIT, "DisconnectNamedPipe failed with %d.\n", GetLastError());
   }

// Call a subroutine to connect to the new client.

   Pipe[i].fPendingIO = ConnectToNewClient(
      Pipe[i].hPipeInst,
      &Pipe[i].oOverlap);

   Pipe[i].dwState = Pipe[i].fPendingIO ?
      CONNECTING_STATE : // still connecting
      READING_STATE;     // ready to read
}

int os_send_command (struct command *cmd, int passfd)
{
	DWORD  cbToWrite, cbWritten;
	OVERLAPPED overlapped;
	BOOL fSuccess;
	struct tlspool_command *t_cmd = &cmd->cmd;

	t_cmd->pio_ancil_type = ANCIL_TYPE_NONE;
	memset (&t_cmd->pio_ancil_data,
			0,
			sizeof (t_cmd->pio_ancil_data));
	/* Send the request */
	// Send a message to the pipe server.

	cbToWrite = sizeof (struct tlspool_command);
	tlog (TLOG_UNIXSOCK, LOG_DEBUG, TEXT("Sending %d byte cmd\n"), cbToWrite);

	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	fSuccess = WriteFile(
		t_cmd->hPipe,                  // pipe handle
		t_cmd,                    // cmd message
		cbToWrite,              // cmd message length
		NULL,                  // bytes written
		&overlapped);            // overlapped

	if (!fSuccess && GetLastError() == ERROR_IO_PENDING )
	{
		tlog (TLOG_UNIXSOCK, LOG_DEBUG, "Write I/O pending\n");
		fSuccess = WaitForSingleObject(overlapped.hEvent, INFINITE) == WAIT_OBJECT_0;
	}

	if (fSuccess) {
		fSuccess = GetOverlappedResult(t_cmd->hPipe, &overlapped, &cbWritten, TRUE);
	}

	if (!fSuccess)
	{
		tlog (TLOG_UNIXSOCK, LOG_CRIT, "WriteFile to pipe failed. GLE=%d\n", GetLastError());
		errno = EPIPE;
		return 0;
	} else {
		tlog (TLOG_UNIXSOCK, LOG_DEBUG, "Wrote %ld bytes to pipe\n", cbWritten);
		return 1;
	}
}

void os_run_service ()
{
   DWORD i, dwWait, cbRet, dwErr;
   BOOL fSuccess;
// The initial loop creates several instances of a named pipe
// along with an event object for each instance.  An
// overlapped ConnectNamedPipe operation is started for
// each instance.

   for (i = 0; i < INSTANCES; i++)
   {

   // Create an event object for this instance.

      hEvents[i] = CreateEvent(
         NULL,    // default security attribute
         TRUE,    // manual-reset event
         TRUE,    // initial state = signaled
         NULL);   // unnamed event object

      if (hEvents[i] == NULL)
      {
         tlog (TLOG_UNIXSOCK, LOG_CRIT, "CreateEvent failed with %d.\n", GetLastError());
      }

      Pipe[i].oOverlap.hEvent = hEvents[i];

      Pipe[i].hPipeInst = CreateNamedPipe(
         szPipename,              // pipe name
         PIPE_ACCESS_DUPLEX |     // read/write access
         FILE_FLAG_OVERLAPPED,    // overlapped mode
         PIPE_TYPE_MESSAGE |      // message-type pipe
         PIPE_READMODE_MESSAGE |  // message-read mode
         PIPE_WAIT,               // blocking mode
         INSTANCES,               // number of instances
         BUFSIZE*sizeof(TCHAR),   // output buffer size
         BUFSIZE*sizeof(TCHAR),   // input buffer size
         PIPE_TIMEOUT,            // client time-out
         NULL);                   // default security attributes

      if (Pipe[i].hPipeInst == INVALID_HANDLE_VALUE)
      {
         tlog (TLOG_UNIXSOCK, LOG_CRIT, "CreateNamedPipe failed with %d.\n", GetLastError());
      }

   // Call the subroutine to connect to the new client

      Pipe[i].fPendingIO = ConnectToNewClient(
         Pipe[i].hPipeInst,
         &Pipe[i].oOverlap);

      Pipe[i].dwState = Pipe[i].fPendingIO ?
         CONNECTING_STATE : // still connecting
         READING_STATE;     // ready to read
   }

   while (1)
   {
   // Wait for the event object to be signaled, indicating
   // completion of an overlapped read, write, or
   // connect operation.

      dwWait = WaitForMultipleObjects(
         INSTANCES,    // number of event objects
         hEvents,      // array of event objects
         FALSE,        // does not wait for all
         INFINITE);    // waits indefinitely

   // dwWait shows which pipe completed the operation.

      i = dwWait - WAIT_OBJECT_0;  // determines which pipe
      if (i < 0 || i > (INSTANCES - 1))
      {
         tlog (TLOG_UNIXSOCK, LOG_CRIT, "Index out of range.\n");
      }

   // Get the result if the operation was pending.

      if (Pipe[i].fPendingIO)
      {
         fSuccess = GetOverlappedResult(
            Pipe[i].hPipeInst, // handle to pipe
            &Pipe[i].oOverlap, // OVERLAPPED structure
            &cbRet,            // bytes transferred
            FALSE);            // do not wait

         switch (Pipe[i].dwState)
         {
         // Pending connect operation
            case CONNECTING_STATE:
               if (! fSuccess)
               {
                   tlog (TLOG_UNIXSOCK, LOG_CRIT, "Error %d.\n", GetLastError());
               }
               tlog (TLOG_UNIXSOCK, LOG_DEBUG, "Connected.\n");
               Pipe[i].dwState = READING_STATE;
               break;

         // Pending read operation
            case READING_STATE:
               if (!fSuccess)
               {
                  tlog (TLOG_UNIXSOCK, LOG_CRIT, "Error GLE = %ld\n", GetLastError());
                  DisconnectAndReconnect(i);
                  continue;
               }
               tlog (TLOG_UNIXSOCK, LOG_DEBUG, "OK cbRet = %d.\n", cbRet);
               Pipe[i].cbRead = cbRet;
				struct command *cmd = allocate_command_for_clientfd((int) &Pipe[i]);
				Pipe[i].chRequest.hPipe = Pipe[i].hPipeInst;
				copy_tls_command (cmd, &Pipe[i].chRequest);
				if (cmd->cmd.pio_ancil_type == ANCIL_TYPE_SOCKET) {
					if (cmd->passfd == -1) {
						//WRONG: no support for sockets
						//HANDLE winsock = (HANDLE) WSASocket(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, &cmd->cmd.pio_ancil_data.pioa_socket, 0, 0);
						//cmd->passfd = cygwin_attach_handle_to_fd(NULL, -1, winsock, NULL, GENERIC_READ | GENERIC_WRITE);
						//tlog (TLOG_UNIXSOCK, LOG_DEBUG, "Received file descriptor as %d, winsock = %d\n", cmd->passfd, winsock);
						cmd->passfd = socket_from_protocol_info(&cmd->cmd.pio_ancil_data.pioa_socket);
						if (cmd->passfd == -1) {
							tlog (TLOG_UNIXSOCK, LOG_CRIT, "WSAGetLastError(void) = %d\n", WSAGetLastError());
						}
						tlog (TLOG_UNIXSOCK, LOG_DEBUG, "Received file descriptor as %d\n", cmd->passfd);
					} else {
						//int superfd = (int) WSASocket(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, &cmd->cmd.pio_ancil_data.pioa_socket, 0, 0);
						//tlog (TLOG_UNIXSOCK, LOG_ERR, "Received superfluous file descriptor as %d", superfd);
						//close (superfd);
					}
				}
				process_command (cmd);

               break;

            default:
            {
               tlog (TLOG_UNIXSOCK, LOG_CRIT, "Invalid pipe state.\n");
            }
         }
      }

   // The pipe state determines which operation to do next.

      switch (Pipe[i].dwState)
      {
      // READING_STATE:
      // The pipe instance is connected to the client
      // and is ready to read a request from the client.

         case READING_STATE:
            fSuccess = ReadFile(
               Pipe[i].hPipeInst,
               &Pipe[i].chRequest,
               sizeof (Pipe[i].chRequest),
               &Pipe[i].cbRead,
               &Pipe[i].oOverlap);

         // The read operation completed successfully.

            if (fSuccess && Pipe[i].cbRead != 0)
            {
				Pipe[i].fPendingIO = FALSE;

               continue;
            }

         // The read operation is still pending.

            dwErr = GetLastError();
            if (! fSuccess && (dwErr == ERROR_IO_PENDING))
            {
               tlog (TLOG_UNIXSOCK, LOG_DEBUG, "read pending. %d\n", sizeof (Pipe[i].chRequest));
               Pipe[i].fPendingIO = TRUE;
               continue;
            }
            tlog (TLOG_UNIXSOCK, LOG_CRIT, "The read failed with %d.\n", GetLastError());

         // An error occurred; disconnect from the client.

            DisconnectAndReconnect(i);
            break;

         default:
         {
            tlog (TLOG_UNIXSOCK, LOG_CRIT, "Invalid pipe state.\n");
         }
      }
  }
}
