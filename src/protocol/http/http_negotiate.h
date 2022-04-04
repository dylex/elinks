
#ifndef EL__PROTOCOL_HTTP_HTTP_NEGOTIATE_H
#define EL__PROTOCOL_HTTP_HTTP_NEGOTIATE_H

#ifdef __cplusplus
extern "C" {
#endif

#define HTTPNEG_GSS		1
#define HTTPNEG_NEG		2

#define HTTPNEG_GSS_STR		"GSS-Negotiate"
#define HTTPNEG_NEG_STR		"Negotiate"

#define HTTPNEG_GSS_STRLEN	sizeof(HTTPNEG_GSS_STR)
#define HTTPNEG_NEG_STRLEN	sizeof(HTTPNEG_NEG_STR)

int http_negotiate_input(struct connection *conn, struct uri *uri,
			int type, char *data);

int http_negotiate_output(struct uri *uri, struct string *header);

#ifdef __cplusplus
}
#endif

#endif /* EL_PROTOCOL_HTTP_HTTP_NEGOTIATE_H */

