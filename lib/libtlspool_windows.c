#include "whoami.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <ctype.h>

#ifndef WINDOWS_PORT
#include <unistd.h>
#endif
#include <pthread.h>
#include <fcntl.h>
#include <syslog.h>

#include <tlspool/starttls.h>
#include <tlspool/commands.h>

#include <winsock2.h>

#define PIPE_TIMEOUT 5000
#define BUFSIZE 4096
#define random rand
#define srandom srand

/* Windows supports SCTP but fails to define this IANA-standardised symbol: */
#ifndef IPPROTO_SCTP
#define IPPROTO_SCTP 132
#endif

static int socket_dup_protocol_info(int fd, int pid, LPWSAPROTOCOL_INFOW lpProtocolInfo)
{
	if (WSADuplicateSocketW((SOCKET)fd, pid, lpProtocolInfo) == SOCKET_ERROR) {
		errno = EPIPE;
		return -1;
	} else {
		return 0;
	}
}

/*
 * converts IPPROTO_* to SOCK_*, returns -1 if invalid protocol
 */
int ipproto_to_sockettype(uint8_t ipproto) {
	return ipproto == IPPROTO_TCP ? SOCK_STREAM : ipproto == IPPROTO_UDP ? SOCK_DGRAM : -1;
}

/*
 * The namedconnect() function is called by tlspool_starttls() when the
 * identities have been exchanged, and established, in the TLS handshake.
 * This is the point at which a connection to the plaintext side is
 * needed, and a callback to namedconnect() is made to find a handle for
 * it.  The function is called with a version of the tlsdata that has
 * been updated by the TLS Pool to hold the local and remote identities. 
 *
 * When the namedconnect argument passed to tlspool_starttls() is NULL,
 * this default function is used instead of the possible override by the
 * caller.  This interprets the privdata handle as an (int *) holding
 * a file descriptor.  If its value is valid, that is, >= 0, it will be
 * returned directly; otherwise, a socketpair is constructed, one of the
 * sockets is stored in privdata for use by the caller and the other is
 * returned as the connected file descriptor for use by the TLS Pool.
 * This means that the privdata must be properly initialised for this
 * use, with either -1 (to create a socketpair) or the TLS Pool's
 * plaintext file descriptor endpoint.  The file handle returned in
 * privdata, if it is >= 0, should be closed by the caller, both in case
 * of success and failure.
 *
 * The return value should be -1 on error, with errno set, or it should
 * be a valid file handle that can be passed back to the TLS Pool to
 * connect to.
 */
int tlspool_namedconnect_default (starttls_t *tlsdata, void *privdata) {
	int plainfd;
	// https://github.com/ncm/selectable-socketpair
	extern int dumb_socketpair(SOCKET socks[2], int sock_type, int make_overlapped);
	SOCKET soxx[2];
	int type = ipproto_to_sockettype (tlsdata->ipproto);
	if (type == -1) {
		errno = EINVAL;
		return -1;
	}
	if (dumb_socketpair(soxx, type, 1) == 0)
	{
		// printf("DEBUG: socketpair succeeded\n");
		/* Socketpair created */
		plainfd = soxx [0];
		* (int *) privdata = soxx [1];
	} else {
		/* Socketpair failed */
		// printf("DEBUG: socketpair failed\n");
		plainfd = -1;
	}
	return plainfd;
}

static pool_handle_t open_named_pipe (LPCTSTR lpszPipename)
{
	HANDLE hPipe;
	//struct tlspool_command chBuf;
	BOOL   fSuccess = FALSE;
	DWORD  dwMode;

	// Try to open a named pipe; wait for it, if necessary.

	while (1)
	{
		hPipe = CreateFile(
			lpszPipename,   // pipe name
			GENERIC_READ |  // read and write access
			GENERIC_WRITE,
			0,              // no sharing
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe
			FILE_FLAG_OVERLAPPED, // overlapped
			NULL);          // no template file

		// Break if the pipe handle is valid.
		if (hPipe != INVALID_POOL_HANDLE)
			break;

		// Exit if an error other than ERROR_PIPE_BUSY occurs.
		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			syslog(LOG_CRIT, "Could not open pipe. GLE=%d\n", GetLastError());
			return INVALID_POOL_HANDLE;
		}

		// All pipe instances are busy, so wait for 20 seconds.
		if (!WaitNamedPipe(lpszPipename, 20000))
		{
			syslog(LOG_CRIT, "Could not open pipe: 20 second wait timed out.");
			return INVALID_POOL_HANDLE;
		}
	}
	// The pipe connected; change to message-read mode.
	dwMode = PIPE_READMODE_MESSAGE;
	fSuccess = SetNamedPipeHandleState(
		hPipe,    // pipe handle
		&dwMode,  // new pipe mode
		NULL,     // don't set maximum bytes
		NULL);    // don't set maximum time
	if (!fSuccess)
	{
		syslog(LOG_CRIT, "SetNamedPipeHandleState failed. GLE=%d\n", GetLastError());
		return INVALID_POOL_HANDLE;
	}
	ULONG ServerProcessId;
	if (GetNamedPipeServerProcessId(hPipe, &ServerProcessId)) {
		syslog(LOG_DEBUG, "DEBUG: GetNamedPipeServerProcessId: ServerProcessId = %ld\n", ServerProcessId);
	} else {
		syslog(LOG_CRIT, "GetNamedPipeServerProcessId failed. GLE=%d\n", GetLastError());
	}
	return hPipe;
}

int os_sendmsg_command(pool_handle_t poolfd, struct tlspool_command *cmd, int fd) {
	DWORD  cbToWrite, cbWritten;
	OVERLAPPED overlapped;
	BOOL fSuccess;

	if (fd >= 0) {
		if (1 /*is_sock(wsock)*/) {
			// Send a socket
			LONG pid;
			
			GetNamedPipeServerProcessId(poolfd, &pid);
			cmd->pio_ancil_type = ANCIL_TYPE_SOCKET;
			syslog(LOG_DEBUG, "DEBUG: pid = %d, fd = %d\n", pid, fd);
			if (socket_dup_protocol_info(fd, pid, &cmd->pio_ancil_data.pioa_socket) == -1) {
				// printf("DEBUG: cygwin_socket_dup_protocol_info error\n");
				// Let SIGPIPE be reported as EPIPE
				closesocket(fd);
				int entry_reqid = cmd->pio_reqid;
				registry_update (&entry_reqid, NULL);
				// errno inherited from socket_dup_protocol_info()
				return -1;
			}
			//... (..., &cmd.pio_ancil_data.pioa_socket, ...);
		} else {
			// Send a file handle
			cmd->pio_ancil_type = ANCIL_TYPE_FILEHANDLE;
			//... (..., &cmd.pio_ancil_data.pioa_filehandle, ...);
		}
	}
	/* Send the request */
	// Send a message to the pipe server.

	cbToWrite = sizeof (struct tlspool_command);
	syslog(LOG_DEBUG, "Sending %d byte cmd\n", cbToWrite);

	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	fSuccess = WriteFile(
		poolfd,                  // pipe handle
		cmd,                    // cmd message
		cbToWrite,              // cmd message length
		NULL,                  // bytes written
		&overlapped);            // overlapped

	if (!fSuccess && GetLastError() == ERROR_IO_PENDING )
	{
// printf ("DEBUG: Write I/O pending\n");
		fSuccess = WaitForSingleObject(overlapped.hEvent, INFINITE) == WAIT_OBJECT_0;
	}

	if (fSuccess) {
		fSuccess = GetOverlappedResult(poolfd, &overlapped, &cbWritten, TRUE);
	}

	if (!fSuccess)
	{
		syslog(LOG_CRIT, "WriteFile to pipe failed. GLE=%d\n", GetLastError());
		errno = EPIPE;
		return -1;
	} else {
// printf ("DEBUG: Wrote %ld bytes to pipe\n", cbWritten);
	}
// printf("DEBUG: Message sent to server, receiving reply as follows:\n");
	return 0;
}

int os_recvmsg_command(pool_handle_t poolfd, struct tlspool_command *cmd) {
	BOOL   fSuccess = FALSE;
	DWORD  cbRead;
	OVERLAPPED overlapped;
	int retval;
	
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Read from the pipe.
	fSuccess = ReadFile(
		poolfd,       // pipe handle
		cmd,         // buffer to receive reply
		sizeof (struct tlspool_command), // size of buffer
		NULL,         // number of bytes read
		&overlapped); // not overlapped

	if (!fSuccess && GetLastError() == ERROR_IO_PENDING )
	{
// printf ("DEBUG: Read I/O pending\n");
		fSuccess = WaitForSingleObject(overlapped.hEvent, INFINITE) == WAIT_OBJECT_0;
	}

	if (fSuccess) {
		fSuccess = GetOverlappedResult(poolfd, &overlapped, &cbRead, TRUE);
	}

	if (!fSuccess)
	{
		syslog(LOG_CRIT, "ReadFile from pipe failed. GLE=%d\n", GetLastError());
		retval = -1;
	} else {
		retval = (int) cbRead;
// printf ("DEBUG: Read %ld bytes from pipe\n", cbRead);
	}
	return retval;
}
pool_handle_t open_pool (void *path) {
// printf ("DEBUG: path = %s\n", (char *) path);
	return open_named_pipe ((LPCTSTR) path);
// printf ("DEBUG: newpoolfd = %d\n", newpoolfd);
}

/* Determine an upper limit for simultaneous STARTTLS threads, based on the
 * number of available file descriptors.  Note: The result is cached, so
 * don't use root to increase beyond max in setrlimit() after calling this.
 */
int tlspool_simultaneous_starttls(void) {
	return 512;
}

int os_usleep(unsigned int usec) {
	Sleep(usec / 1000);
	return 0;
}
