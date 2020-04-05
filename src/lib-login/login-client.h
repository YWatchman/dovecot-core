#ifndef LOGIN_CLIENT_H
#define LOGIN_CLIENT_H

#include "net.h"

struct master_service;

/* Authentication client process's cookie size */
#define LOGIN_REQUEST_COOKIE_SIZE (128/8)

/* LOGIN_MAX_INBUF_SIZE should be based on this. Keep this large enough so that
   LOGIN_MAX_INBUF_SIZE will be 1024+2 bytes. This is because IMAP ID command's
   values may be max. 1024 bytes plus 2 for "" quotes. (Although it could be
   even double of that when value is full of \" quotes, but for now lets not
   make it too easy to waste memory..) */
#define LOGIN_REQUEST_MAX_DATA_SIZE (1024 + 128 + 64 + 2)

#define LOGIN_REQUEST_ERRMSG_INTERNAL_FAILURE \
	"Internal error occurred. Refer to server log for more information."

enum login_request_flags {
	/* Connection has TLS compression enabled */
	LOGIN_REQUEST_FLAG_TLS_COMPRESSION	= BIT(0),
	/* Connection is secure (SSL or just trusted) */
	LOGIN_REQUEST_FLAG_CONN_SECURED		= BIT(1),
	/* Connection is secured using SSL specifically */
	LOGIN_REQUEST_FLAG_CONN_SSL_SECURED	= BIT(2),
	/* This login is implicit; no command reply is expected */
	LOGIN_REQUEST_FLAG_IMPLICIT		= BIT(3),
};

/* Login request. File descriptor may be sent along with the request. */
struct login_request {
	/* Request tag. Reply is sent back using same tag. */
	unsigned int tag;

	/* Authentication process, authentication ID and auth cookie. */
	pid_t auth_pid;
	unsigned int auth_id;
	unsigned int client_pid;
	uint8_t cookie[LOGIN_REQUEST_COOKIE_SIZE];

	/* Properties of the connection. The file descriptor
	   itself may be a local socketpair. */
	struct ip_addr local_ip, remote_ip;
	in_port_t local_port, remote_port;

	uint32_t flags;

	/* request follows this many bytes of client input */
	uint32_t data_size;
	/* inode of the transferred fd. verified just to be sure that the
	   correct fd is mapped to the correct struct. */
	ino_t ino;
};

enum login_reply_status {
	LOGIN_REPLY_STATUS_OK,
	LOGIN_REPLY_STATUS_INTERNAL_ERROR
};

struct login_reply {
	/* tag=0 are notifications from master */
	unsigned int tag;
	enum login_reply_status status;
	/* PID of the post-login mail process handling this connection */
	pid_t mail_pid;
};

struct login_client_request_params {
	/* Client fd to transfer to post-login process or -1 if no fd is
	   wanted to be transferred. */
	int client_fd;
	/* Override login_connection_list->default_path if non-NULL */
	const char *socket_path;

	/* Login request that is sent to post-login process.
	   tag is ignored. */
	struct login_request request;
	/* Client input of size request.data_size */
	const unsigned char *data;
};

/* reply=NULL if the login was cancelled due to some error */
typedef void login_client_request_callback_t(const struct login_reply *reply,
					     void *context);

struct login_client_list *
login_client_list_init(struct master_service *service, const char *path);
void login_client_list_deinit(struct login_client_list **list);

/* Send a login request. Returns tag which can be used to abort the
   request (ie. ignore the reply from master). */
void login_client_request(struct login_client_list *list,
			  const struct login_client_request_params *params,
			  login_client_request_callback_t *callback,
			  void *context, unsigned int *tag_r);
void login_client_request_abort(struct login_client_list *list,
				unsigned int tag);

#endif