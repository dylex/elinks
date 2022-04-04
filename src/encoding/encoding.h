#ifndef EL__ENCODING_ENCODING_H
#define EL__ENCODING_ENCODING_H

#include "network/state.h"
#include "util/string.h"

#ifdef __cplusplus
extern "C" {
#endif

enum stream_encoding {
	ENCODING_NONE = 0,
	ENCODING_GZIP,
	ENCODING_BZIP2,
	ENCODING_LZMA,
	ENCODING_BROTLI,
	ENCODING_ZSTD,

	/* Max. number of known encoding including ENCODING_NONE. */
	ENCODINGS_KNOWN,
};

struct stream_encoded {
	enum stream_encoding encoding;
	void *data;
};

struct decoding_backend {
	const char *name;
	const char *const *extensions;
	int (*open)(struct stream_encoded *stream, int fd);
	int (*read)(struct stream_encoded *stream, char *data, int len);
	char *(*decode_buffer)(struct stream_encoded *stream, char *data, int len, int *new_len);
	void (*close)(struct stream_encoded *stream);
};

struct stream_encoded *open_encoded(int, enum stream_encoding);
int read_encoded(struct stream_encoded *, char *, int);
char *decode_encoded_buffer(struct stream_encoded *stream, enum stream_encoding encoding, char *data, int len, int *new_len);
void close_encoded(struct stream_encoded *);

const char *const *listext_encoded(enum stream_encoding);
enum stream_encoding guess_encoding(char *filename);
const char *get_encoding_name(enum stream_encoding encoding);

/* Read from open @stream into the @page string */
struct connection_state
read_file(struct stream_encoded *stream, int readsize, struct string *page);

/* Reads the file with the given @filename into the string @source. */
struct connection_state read_encoded_file(struct string *filename, struct string *source);

#ifdef __cplusplus
}
#endif

#endif
