#ifndef EL__ECMASCRIPT_QUICKJS_H
#define EL__ECMASCRIPT_QUICKJS_H

#include <quickjs/quickjs.h>

#ifdef ECMASCRIPT_DEBUG

#define RETURN_JS(obj) \
	fprintf(stderr, "%s:%d obj=%p\n", __FILE__, __LINE__, JS_VALUE_GET_PTR(obj)); \
    if (JS_VALUE_HAS_REF_COUNT(obj)) { \
        JSRefCountHeader *p = (JSRefCountHeader *)JS_VALUE_GET_PTR(obj); \
        fprintf(stderr, "ref_count=%d\n", p->ref_count); \
    } \
	return obj

#else

#define RETURN_JS(obj) return obj

#endif

struct ecmascript_interpreter;
struct form_view;
struct form_state;
struct string;

void *quickjs_get_interpreter(struct ecmascript_interpreter *interpreter);
void quickjs_put_interpreter(struct ecmascript_interpreter *interpreter);

void quickjs_detach_form_view(struct form_view *fv);
void quickjs_detach_form_state(struct form_state *fs);
void quickjs_moved_form_state(struct form_state *fs);

void quickjs_eval(struct ecmascript_interpreter *interpreter, struct string *code, struct string *ret);
char *quickjs_eval_stringback(struct ecmascript_interpreter *interpreter, struct string *code);
int quickjs_eval_boolback(struct ecmascript_interpreter *interpreter, struct string *code);

void quickjs_call_function(struct ecmascript_interpreter *interpreter, JSValueConst fun, struct string *ret);

#ifndef JS_NAN_BOXING
inline int operator<(JSValueConst a, JSValueConst b)
{
	return JS_VALUE_GET_PTR(a) < JS_VALUE_GET_PTR(b);
}
#endif

extern struct module quickjs_module;
#endif
