/* libpavlov -- expect/response interaction to replace chat(8)
 *
 * We use a few expect/response notations in the TLS Pool, and relied on
 * chat(8) which did such things for pppd.  This was less than perfect.  We now
 * change that to libpavlov, with a straightforward extension to a main program,
 * that allows us to play interactive scripts with some degree of flexibility.
 *
 * This is used in:
 *  - tlstunnel                  --> to initiate a STARTTLS sequence
 *  - testcli, testsrv, testpeer --> to play standardised interactions over TLS
 *
 * The general idea for the command set is documented in an issue:
 * https://github.com/arpa2/tlspool/issues/110
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include <errno.h>
// #include <com_err.h>
// #include <com_err/pavlov.h>

#include <sys/types.h>
#include <regex.h>

#if  defined(__CYGWIN__) || defined(__MINGW64__) || defined(_WIN32)
#define WINDOWS_PORT
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

typedef u_short sa_family_t;

static int readable_timeo(int fd, int sec) {
	fd_set rset;
	struct timeval tv;
	
	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	
	tv.tv_sec = sec;
	tv.tv_usec = 0;
	
	return select(fd + 1, &rset, NULL, NULL, sec == 0 ? NULL : &tv);
		/* > 0 if descriptor available */
}

static int writable_timeo(int fd, int sec) {
	fd_set wset;
	struct timeval tv;
	
	FD_ZERO(&wset);
	FD_SET(fd, &wset);
	
	tv.tv_sec = sec;
	tv.tv_usec = 0;
	
	return select(fd + 1, NULL, &wset, NULL, sec == 0 ? NULL : &tv);
		/* > 0 if descriptor available */
}

/* Main interaction point: pavlov () with parameters:
 *
 *  - fdin, fdout --> file descriptors for input/output streams
 *  - progname, argc, argv  --> as in main, but argv[0] consumed into progname
 *
 * The return value is like a value for exit() -- 0 for ok, or 1 with errno.
 *
 * Commands start from argv[0] and contain, among others:
 *
 *   - !<data> --> send out the data
 *   - ?<regex> --> receive >0 bytes and recognise the POSIX extended regex
 *   - @<msdelay> --> delay in milli-seconds
 */
int pavlov (int fdin, int fdout,
		char *progname, int argc, char *argv[], int timeout) {
	//
	// Iterate through arguments, starting at argv [0]
	char databuf [4096 + 1];
	int argi = 0;
	
	while (argi < argc) {
		char *arg = argv [argi];
		if (arg == NULL) {
			// errno = PAVLOV_ILLEGAL_STRUCTURE;
			goto fail;
		}
		char cmd = *arg++;
		switch (cmd) {
		//
		// Command "handler" for no command at all
		case '\0':
			// errno = PAVLOV_ILLEGAL_STRUCTURE;
			goto fail;
		//
		// Command handler for writing output
		case '!':
			{ }
#ifndef WINDOWS_PORT
			int written = writable_timeo(fdout, timeout) > 0 ? write (fdout, arg, strlen (arg)) : -1;
#else
			int written = writable_timeo(fdout, timeout) > 0 ? send (fdout, arg, strlen (arg), 0) : -1;
#endif			
			if (written < 0)  {
				// errno = PAVLOV_OUTPUT_BROKEN;
				goto fail;
			}
			break;
		//
		// Command handler for extended regular extensions
		case '?':
			{ }
			regex_t rexi;
			if (regcomp (&rexi, arg, REG_EXTENDED | REG_NEWLINE | REG_NOSUB) != 0) {
				/* errno is set */
				goto fail;
			}
			bool busy = true;
			do {
#ifndef WINDOWS_PORT
				int datalen = readable_timeo(fdin, timeout) > 0 ? read (fdin, databuf, sizeof (databuf) - 1) : -1;
#else
				int datalen = readable_timeo(fdin, timeout) > 0 ? recv (fdin, databuf, sizeof (databuf) - 1, 0) : -1;
#endif			
				if (datalen < 0) {
					// errno = PAVLOV_INPUT_BROKEN;
					break;
				}
				if (memchr (databuf, '\0', datalen) != NULL) {
					// errno = PAVLOV_INPUT_BINARY;
					break;
				}
				databuf [datalen] = '\0';
				if (regexec (&rexi, databuf, 0, NULL, 0) == 0) {
					busy = false;
				}
			} while (busy);
			regfree (&rexi);
			if (busy) {
				goto fail;
			}
			break;
		//
		// Command handler for microsecond delays
		case '@':
			{ }
			long msdelay = strtol (arg, &arg, 10);
			if (*arg != '\0') {
				errno = EINVAL;
				// errno = PAVLOV_TIMEOUT_SYNTAX
				goto fail;
			}
			useconds_t usleep_delay = msdelay * 1000;
			if (usleep_delay / 1000 != msdelay) {
				errno = ERANGE;
				// errno = PAVLOV_TIMEOUT_SYNTAX
				goto fail;
			}
			usleep (usleep_delay);
			break;
		//
		// Default handling for unknown commands
		default:
			errno = EINVAL;
			// errno = PAVLOV_UNRECOGNISED_COMMAND;
			goto fail;
		//
		// End of command switch
		}
		argi++;
	}
	//
	// Done.  Return success.
	return 0;
	//
	// Return in turmoil
fail:
	//TODO// implement an error table for pavlov, and allow errno = PAVLOV_xxx above
	errno = EINVAL;
	return 1;
}
