#ifndef SERVICE_H
#define SERVICE_H
extern int stop_service;
extern struct callback cblist [1024];
extern struct callback *cbfree;
extern pthread_mutex_t cbfree_mutex;

struct command *allocate_command_for_clientfd (pool_handle_t fd);
void copy_tls_command(struct command *cmd, struct tlspool_command *tls_command);
void process_command (struct command *cmd);
void free_commands_by_clientfd (pool_handle_t clientfd);

int os_send_command (struct command *cmd, int passfd);
void os_run_service ();

#ifndef WINDOWS_PORT
int is_callback (struct command *cmd);
void post_callback (struct command *cmd);
#endif /* WINDOWS_PORT */
#endif /* SERVICE_H */
