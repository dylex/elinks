#ifndef EL__ECMASCRIPT_MUJS_DOCUMENT_H
#define EL__ECMASCRIPT_MUJS_DOCUMENT_H

#include <mujs.h>

void mjs_push_document(js_State *J, void *doc);
int mjs_document_init(js_State *J);

#endif
