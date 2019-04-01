/* The request registry is an array of pointers, filled by the starttls_xxx()
 * functions for as long as they have requests standing out.  The registry
 * permits instant lookup of a mutex to signal, so the receiving end may
 * pickup the message in its also-registered tlspool command buffer.
 */
struct registry_entry {
	pthread_mutex_t *sig;		/* Wait for master thread's recvmsg() */
	struct tlspool_command *buf;	/* Buffer to hold received command */
	pool_handle_t pfd;			/* Client thread's assumed poolfd */
};

int os_sendmsg_command(pool_handle_t poolfd, struct tlspool_command *cmd, int fd);
int os_recvmsg_command(pool_handle_t poolfd, struct tlspool_command *cmd);
int tlspool_namedconnect_default (starttls_t *tlsdata, void *privdata);
int tlspool_simultaneous_starttls(void);
int os_sleep(unsigned int usec);
int registry_update (int *reqid, struct registry_entry *entry);
int ipproto_to_sockettype(uint8_t ipproto);
pool_handle_t open_pool (void *path);
