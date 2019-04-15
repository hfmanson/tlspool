/* tlspool/libfun.c -- Library function for starttls go-get-it */

#include "whoami.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>

#include <unistd.h>
#include <pthread.h>

#include <tlspool/starttls.h>
#include <tlspool/commands.h>

int tlspool_execute_command (char *path, struct tlspool_command *cmd, int expected_response);

/* The library function to service local identity callbacks.  It registers
 * with the TLS Pool and will service callback requests until it is no
 * longer welcomed.  Of course, if another process already has a claim on
 * this functionality, the service offering will not be welcome from the
 * start.
 *
 * This function differs from most other TLS Pool library functions in
 * setting up a private socket.  This helps to avoid the overhead in the
 * foreseeable applications that only do this; it also helps to close
 * down the exclusive claim on local identity resolution when (part of)
 * the program is torn down.  The function has been built to cleanup
 * properly when it is subjected to pthread_cancel().
 *
 * The path parameter offers a mechanism to specify the socket path.  When
 * set to NULL, the library's compiled-in default path will be used.
 *
 * In terms of linking, this routine is a separate archive object.  This
 * minimizes the amount of code carried around in statically linked binaries.
 *
 * This function returns -1 on error, or 0 on success.
 */
int tlspool_localid_service (char *path, uint32_t regflags, int responsetimeout, char * (*cb) (lidentry_t *entry, void *data), void *data) {
	pool_handle_t pool_handle = INVALID_POOL_HANDLE;
	struct tlspool_command cmd;
	char *cberr = NULL;
	int  rc = 0;
	
	/* Prepare command structure */
	memset (&cmd, 0, sizeof (cmd));	/* Do not leak old stack info */
	cmd.pio_cbid = 0;
	cmd.pio_cmd = PIOC_LIDENTRY_REGISTER_V2;
	cmd.pio_data.pioc_lidentry.flags = regflags;
	cmd.pio_data.pioc_lidentry.timeout = responsetimeout;

	/* Loop forever... or until an error occurs */
	while (1) {

		/* send the request or, when looping, the callback result */
//DEBUG// printf ("DEBUG: LIDENTRY command 0x%08lx with cbid=%d and flags 0x%08lx\n", cmd.pio_cmd, cmd.pio_cbid, cmd.pio_data.pioc_lidentry.flags);
		rc = tlspool_execute_command (path, &cmd, PIOC_LIDENTRY_CALLBACK_V2);
//DEBUG// printf ("DEBUG: LIDENTRY callback command 0x%08lx with cbid=%d and flags 0x%08lx\n", cmd.pio_cmd, cmd.pio_cbid, cmd.pio_data.pioc_lidentry.flags);
		if (rc == 0) {
			cberr = cb (&cmd.pio_data.pioc_lidentry, data);
			if (cberr != NULL) {
				cmd.pio_cmd = PIOC_ERROR_V2;
				cmd.pio_data.pioc_error.tlserrno = errno;
				strncpy (cmd.pio_data.pioc_error.message,
					cberr,
					sizeof (cmd.pio_data.pioc_error.message));
				// Next loop will send error to TLS Pool, which will
				// bounce the error back to us so we can report it too
			}
		}
	}

	/* Never end up here... */
	//pthread_cleanup_pop (1);
	return rc;
}
