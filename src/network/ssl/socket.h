
#ifndef EL__NETWORK_SSL_SOCKET_H
#define EL__NETWORK_SSL_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SSL

struct socket;

int ssl_connect(struct socket *socket);
ssize_t ssl_write(struct socket *socket, char *data, int len);
ssize_t ssl_read(struct socket *socket, char *data, int len);
int ssl_close(struct socket *socket);

#endif

#ifdef __cplusplus
}
#endif

#endif
