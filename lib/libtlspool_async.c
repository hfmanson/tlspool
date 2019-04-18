/* Asynchronous client API for the TLS Pool
 *
 * This is a minimalistic wrapper around the communication
 * with the TLS Pool.  Its functions are limited to passing
 * data structures from and to sockets, and giving acces
 * to event handler libraries.
 *
 * This interface intends to be portable.  It also aims
 * to be minimal, making it useful for two future directions:
 * networked TLS Pools and embedded/small TLS Pool support.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <tlspool/async.h>
#include <tlspool/starttls.h>

#ifdef WINDOWS_PORT
#define random rand
#define srandom srand
int os_recvmsg_command_no_wait(int poolfd, struct tlspool_command *cmd);
#else
#include <sys/socket.h>
#endif /* WINDOWS_PORT */

int open_pool (void *path);
int os_sendmsg_command(int poolfd, struct tlspool_command *cmd, int fd);
int os_recvmsg_command(int poolfd, struct tlspool_command *cmd);

/* Initialise a new asynchronous TLS Pool handle.
 * This opens a socket, embedded into the pool
 * structure.
 *
 * It is common, and therefore supported in this
 * call, to start with a "ping" operation to
 * exchange version information and supported
 * facilities.  Since this is going to be in all
 * code, where it would be difficult due to the
 * asynchronous nature of the socket, we do it
 * here, just before switching to asynchronous
 * mode.  It is usually not offensive to bootstrap
 * synchronously, but for some programs it may
 * incur a need to use a thread pool to permit
 * the blocking wait, or (later) reconnects can
 * simply leave the identity to provide NULL and
 * not TLSPOOL_IDENTITY_V2 which you would use to
 * allow this optional facility.  We will ask
 * for PIOF_FACILITY_ALL_CURRENT but you want to
 * enforce less, perhaps PIOF_FACILITY_STARTTLS,
 * as requesting too much would lead to failure
 * opening the connection to the TLS Pool.
 *
 * Return true on success, false with errno on failure.
 */
bool tlspool_async_open (struct tlspool_async_pool *pool,
			size_t sizeof_tlspool_command,
			char *tlspool_identity,
			uint32_t required_facilities,
			char *socket_path) {
	int pool_handle = -1;
	//
	// Validate expectations of the caller
	if (sizeof (struct tlspool_command) != sizeof_tlspool_command) {
		errno = EPROTO;
		return false;
	}
	//
	// Find the socket_path to connect to
	if (socket_path == NULL) {
		socket_path = tlspool_configvar (NULL, "socket_name");
	}
	if (socket_path == NULL) {
		socket_path = TLSPOOL_DEFAULT_SOCKET_PATH;
	}
	//
	// Initialise the structure with basic data
	memset (pool, 0, sizeof (*pool));
	pool->cmdsize = sizeof_tlspool_command;
	//
	// Open the handle to the TLS Pool
	pool_handle = open_pool (socket_path);
	if (pool_handle < 0) {
		goto fail_handle;
	}
	//
	// If requested, perform a synchronous ping.
	// This code is needed once in any program,
	// and blocking is easier.  Also, blocking
	// is not as offensive during initialisation
	// as later on, and this is quite simple.
	if (required_facilities != 0) {
		struct tlspool_command poolcmd;
		memset (&poolcmd, 0, sizeof (poolcmd));	/* Do not leak old stack info */
		poolcmd.pio_reqid = 1;
		poolcmd.pio_cbid = 0;
		poolcmd.pio_cmd = PIOC_PING_V2;
		//
		// Tell the TLS Pool what we think of them, and what we would like to have
		assert (strlen (tlspool_identity) < sizeof (poolcmd.pio_data.pioc_ping.YYYYMMDD_producer));
		strcpy (poolcmd.pio_data.pioc_ping.YYYYMMDD_producer, tlspool_identity);
		poolcmd.pio_data.pioc_ping.facilities = required_facilities | PIOF_FACILITY_ALL_CURRENT;
		//
		// Send the request and await its response -- no contenders makes life easy
		if (os_sendmsg_command (pool_handle, &poolcmd, -1) == -1) {
			errno = EPROTO;
			goto fail_handle_close;
		}

		if (os_recvmsg_command(pool_handle, &poolcmd) != sizeof (poolcmd)) {
			errno = EPROTO;
			goto fail_handle_close;
		}
		//
		// In any case, return the negotiated data; then be sure it meets requirements
		memcpy (&pool->pingdata, &poolcmd.pio_data.pioc_ping, sizeof (struct pioc_ping));
		if ((pool->pingdata.facilities & required_facilities) != required_facilities) {
			errno = ENOSYS;
			goto fail_handle_close;
		}
	}
#ifndef WINDOWS_PORT	
	//
	// Make the connection non-blocking
	int soxflags = fcntl (pool_handle, F_GETFL, 0);
	if (fcntl (pool_handle, F_SETFL, soxflags | O_NONBLOCK) != 0) {
		goto fail_handle_close;
	}
#endif	
	//
	// Report success
	pool->handle = pool_handle;
	return true;
	//
	// Report failure
fail_handle_close:
	tlspool_close_poolhandle (pool_handle);
fail_handle:
	pool->handle = -1;
	return false;
}


/* Send a request to the TLS Pool and register a
 * callback handle for it.
 *
 * Return true on succes, false with errno on failure.
 */
bool tlspool_async_request (struct tlspool_async_pool *pool,
			struct tlspool_async_request *reqcb,
			int opt_fd) {
	//
	// Consistency is better checked now than later
	assert (reqcb->cbfunc != NULL);
	//
	// Loop until we have a unique reqid
	bool not_done = true;
	do {
		uint16_t reqid = (uint16_t) (random() & 0x0000ffff);
		reqcb->cmd.pio_reqid = reqid;
		struct tlspool_async_request *prior_entry = NULL;
		HASH_FIND (hh, pool->requests, &reqid, 2, prior_entry);
		//LIST_STYLE// DL_SEARCH_SCALAR (pool->requests, prior_entry, cmd.pio_reqid, reqid);
		not_done = (prior_entry != NULL);
	} while (not_done);
	//
	// Insert into the hash table with the unique reqid
	HASH_ADD (hh, pool->requests, cmd.pio_reqid, 2, reqcb);
	//LIST_STYLE// DL_APPEND (pool->requests, reqcb);
	//
	// Construct the message to send -- including opt_fd, if any
	ssize_t sent = os_sendmsg_command (pool->handle, &reqcb->cmd, opt_fd);
	if (sent < sizeof (struct tlspool_command)) {
		/* Sending failed; drill down to see why */
		if (sent == 0) {
			/* This is not a problem, we can try again later */
			errno = EAGAIN;
			goto fail;
		} else if (sent < 0) {
			/* We got an errno value to return; socket is ok */
			goto fail;
		}
		/* Sending to the socket is no longer reliable */
#ifndef WINDOWS_PORT		
		shutdown (pool->handle, SHUT_WR);
#else
		tlspool_close_poolhandle (pool->handle);
#endif
		errno = EPROTO;
		goto fail;
	}
	//
	// Return successfully
	return true;
	//
	// Return failure -- and always close the opt_fd, if any
fail:
	if (opt_fd >= 0) {
		close (opt_fd);
	}
	return false;
}


/* Cancel a request.  Do not trigger the callback.
 *
 * BE CAREFUL.  The TLS Pool can still send back a
 * response with the request identity, and you have
 * no way of discovering that if it arrives at a new
 * request.  EMBRACE FOR IMPACT.
 *
 * Return true on success, false with errno otherwise.
 */
bool tlspool_async_cancel (struct tlspool_async_pool *pool,
			struct tlspool_async_request *reqcb) {
	//
	// Just rip the request from the hash, ignoring
	// what it will do when a response comes back
	HASH_DEL (pool->requests, reqcb);
	//LIST_STYLE// DL_DELETE (pool->requests, reqcb);
	//
	// No sanity checks; you are not sane using this...
	return true;
}


/* Process all (but possibly no) outstanding requests
 * by reading any available data from the TLS Pool.
 *
 * Return true on success, false with errno otherwise.
 *
 * Specifically, when errno is EAGAIN or EWOULDBLOCK,
 * the return value is true to indicate business as
 * usual.
 */
bool tlspool_async_process (struct tlspool_async_pool *pool) {
	//
	// Start a loop reading from the TLS Pool	
	while (true) {
		//
		// Reception structures
		// Note that we do not receive file descriptors
		// (maybe later -- hence opt_fd in the callback)
		struct tlspool_command poolcmd;
		//
		// Prepare memory structures for reception
		memset (&poolcmd, 0, sizeof (poolcmd));   /* never, ever leak stack data */
		// Receive the message and weigh the results
#ifndef WINDOWS_PORT		
		ssize_t recvd = os_recvmsg_command(pool->handle, &poolcmd);
#else
		ssize_t recvd = os_recvmsg_command_no_wait(pool->handle, &poolcmd);
#endif
		if (recvd < sizeof (struct tlspool_command)) {
			/* Reception failed; drill down to see why */
			if (recvd == 0) {
				/* This is not a problem, we can try again */
				return true;
			} else if (recvd < 0) {
				/* We got an errno to pass; socket is ok */
				if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
					errno = 0;
					return true;
				} else {
					return false;
				}
			}
			/* Receiving from the socket is no longer reliable */
			tlspool_close_poolhandle (pool->handle);
			pool->handle = -1;
			errno = EPROTO;
			return false;
		}
		//
		// Find the callback routine
		struct tlspool_async_request *reqcb = NULL;
		HASH_FIND (hh, pool->requests, &poolcmd.pio_reqid, 2, reqcb);
		//LIST_STYLE// DL_SEARCH_SCALAR (pool->requests, reqcb, cmd.pio_reqid, poolcmd.pio_reqid);
		if (reqcb == NULL) {
			/* We do not have a callback, user should decide */
			errno = ENOENT;
			return false;
		}
		//
		// Take the callback function out of the hash
		HASH_DEL (pool->requests, reqcb);
		//LIST_STYLE// DL_DELETE (pool->requests, reqcb);
		//
		// Clone the command structure to the request structure
		memcpy (&reqcb->cmd, &poolcmd, sizeof (reqcb->cmd));
		//
		// Invoke the callback; currently, we never receive an opt_fd
		reqcb->cbfunc (reqcb, -1);
		//
		// Continue processing with the next entry
	}
}


/* Indicate that a connection to the TLS Pool has been
 * closed down.  Cancel any pending requests by locally
 * generating error responses.
 *
 * Return true on success, false with errno otherwise.
 */
bool tlspool_async_close (struct tlspool_async_pool *pool,
				bool close_socket) {
	//
	// Should we try to close the underlying socket
	if (close_socket && (pool->handle != -1)) {
		tlspool_close_poolhandle (pool->handle);
	}
	//
	// Locally clone the hash of pending requests
	struct tlspool_async_request *stopit = pool->requests;
	pool->requests = NULL;
	//
	// Iterate over hash elements and callback on them
	struct tlspool_async_request *here, *_tmp;
	HASH_ITER (hh, stopit, here, _tmp)
	//LIST_STYLE// DL_FOREACH_SAFE (stopit, here, _tmp)
	{
		//
		// Remove the entry from the cloned hash
		HASH_DEL (stopit, here);
		//LIST_STYLE// DL_DELETE (stopit, here);
		//
		// Fill the cmd buffer with an error message
		here->cmd.pio_cmd = PIOC_ERROR_V2;
		here->cmd.pio_data.pioc_error.tlserrno = EPIPE;
		strncpy (here->cmd.pio_data.pioc_error.message,
			"Disconnected from the TLS Pool",
			sizeof (here->cmd.pio_data.pioc_error.message));
		//
		// Invoke the callback to process the error
		here->cbfunc (here, -1);
	}
	//
	// Return success.
	return true;
}


//TODO// How to register with an event loop?  The int is strange on Windows...

