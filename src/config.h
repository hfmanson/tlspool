enum VARS {
	CFGVAR_DAEMON_PIDFILE,
	CFGVAR_SOCKET_USER,
	CFGVAR_SOCKET_GROUP,
	CFGVAR_SOCKET_MODE,
	CFGVAR_PKCS11_PATH,
	CFGVAR_PKCS11_PIN,
	CFGVAR_CACHE_TTL,
	CFGVAR_CACHE_PORT,
	CFGVAR_PRIVACY_ATTEMPT,
	CFGVAR_LDAP_PROXY,
	CFGVAR_RADIUS_AUTHN,
	CFGVAR_RADIUS_AUTHZ,
	CFGVAR_RADIUS_ACCT,
	CFGVAR_LOG_LEVEL,
	CFGVAR_LOG_FILTER,
	CFGVAR_LOG_STDERR,
	CFGVAR_DBENV_DIR,
	CFGVAR_DB_LOCALID,
	CFGVAR_DB_DISCLOSE,
	CFGVAR_DB_TRUST,
	CFGVAR_TLS_DHPARAMFILE,
	CFGVAR_TLS_MAXPREAUTH,
	CFGVAR_TLS_ONTHEFLY_SIGNCERT,
	CFGVAR_TLS_ONTHEFLY_SIGNKEY,
	CFGVAR_FACILITIES_DENY,
	CFGVAR_FACILITIES_ALLOW,
	CFGVAR_DNSSEC_ROOTKEY,
	CFGVAR_KRB_CLIENT_KEYTAB,
	CFGVAR_KRB_SERVER_KEYTAB,
	CFGVAR_KRB_CLIENT_CREDCACHE,
	CFGVAR_KRB_SERVER_CREDCACHE,
	//
	CFGVAR_LENGTH,
	CFGVAR_NONE = -1
};

extern char *configvars [];

void cfg_setvar (char *item, int itemno, char *value);
void cfg_socketname (char *item, int itemno, char *value);
void cfg_p11path (char *item, int itemno, char *value);
void cfg_p11token (char *item, int itemno, char *value);
void cfg_ldap (char *item, int itemno, char *value);
void cfg_cachehost (char *item, int itemno, char *value);
#ifndef WINDOWS_PORT
extern int kill_old_pid;
void cfg_pidfile (char *item, int itemno, char *value);
void cfg_user (char *item, int itemno, char *value);
void cfg_group (char *item, int itemno, char *value);
void cfg_chroot (char *item, int itemno, char *value);
#endif
