#ifndef EL__DOCUMENT_GEMINI_RENDERER_H
#define EL__DOCUMENT_GEMINI_RENDERER_H

#ifdef __cplusplus
extern "C" {
#endif

struct cache_entry;
struct document;
struct string;

void render_gemini_document(struct cache_entry *cached, struct document *document, struct string *buffer);

#ifdef __cplusplus
}
#endif

#endif
