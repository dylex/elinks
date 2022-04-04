/* The QuickJS domimplementation object. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elinks.h"

#include "ecmascript/ecmascript.h"
#include "ecmascript/quickjs.h"
#include "ecmascript/quickjs/document.h"
#include "ecmascript/quickjs/implementation.h"
#include "util/conv.h"

#include <libxml/HTMLparser.h>
#include <libxml++/libxml++.h>

#define countof(x) (sizeof(x) / sizeof((x)[0]))

static JSClassID js_implementation_class_id;

static JSValue
js_implementation_createHTMLDocument(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s\n", __FILE__, __FUNCTION__);
#endif
	if (argc != 1) {
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s %d\n", __FILE__, __FUNCTION__, __LINE__);
#endif
		return JS_UNDEFINED;
	}
	size_t len;
	const char *title = JS_ToCStringLen(ctx, &len, argv[0]);

	if (!title) {
		return JS_EXCEPTION;
	}
	struct string str;

	if (!init_string(&str)) {
		JS_FreeCString(ctx, title);

		return JS_NULL;
	}
	add_to_string(&str, "<!doctype html>\n<html><head><title>");
	add_html_to_string(&str, title, len);
	add_to_string(&str, "</title></head><body></body></html>");

	// Parse HTML and create a DOM tree
	xmlDoc* doc = htmlReadDoc((xmlChar*)str.source, NULL, "utf-8",
	HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
	// Encapsulate raw libxml document in a libxml++ wrapper
	xmlpp::Document *docu = new(std::nothrow) xmlpp::Document(doc);
	done_string(&str);
	JS_FreeCString(ctx, title);

	return getDocument(ctx, docu);
}

static JSValue
js_implementation_toString(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s\n", __FILE__, __FUNCTION__);
#endif
	return JS_NewString(ctx, "[implementation object]");
}

static const JSCFunctionListEntry js_implementation_proto_funcs[] = {
	JS_CFUNC_DEF("createHTMLDocument", 1, js_implementation_createHTMLDocument),
	JS_CFUNC_DEF("toString", 0, js_implementation_toString)
};

static JSClassDef js_implementation_class = {
	"implementation",
};

static JSValue
js_implementation_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)
{
	JSValue obj = JS_UNDEFINED;
	JSValue proto;
	/* using new_target to get the prototype is necessary when the
	 class is extended. */
	proto = JS_GetPropertyStr(ctx, new_target, "prototype");

	if (JS_IsException(proto)) {
		goto fail;
	}
	obj = JS_NewObjectProtoClass(ctx, proto, js_implementation_class_id);
	JS_FreeValue(ctx, proto);

	if (JS_IsException(obj)) {
		goto fail;
	}
	RETURN_JS(obj);

fail:
	JS_FreeValue(ctx, obj);
	return JS_EXCEPTION;
}

int
js_implementation_init(JSContext *ctx, JSValue global_obj)
{
	JSValue implementation_proto, implementation_class;

	/* create the implementation class */
	JS_NewClassID(&js_implementation_class_id);
	JS_NewClass(JS_GetRuntime(ctx), js_implementation_class_id, &js_implementation_class);

	implementation_proto = JS_NewObject(ctx);
	JS_SetPropertyFunctionList(ctx, implementation_proto, js_implementation_proto_funcs, countof(js_implementation_proto_funcs));

	implementation_class = JS_NewCFunction2(ctx, js_implementation_ctor, "implementation", 0, JS_CFUNC_constructor, 0);
	/* set proto.constructor and ctor.prototype */
	JS_SetConstructor(ctx, implementation_class, implementation_proto);
	JS_SetClassProto(ctx, js_implementation_class_id, implementation_proto);

	JS_SetPropertyStr(ctx, global_obj, "implementation", implementation_proto);
	return 0;
}

JSValue
getImplementation(JSContext *ctx)
{
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s\n", __FILE__, __FUNCTION__);
#endif
	JSValue implementation_obj = JS_NewObject(ctx);
	JS_SetPropertyFunctionList(ctx, implementation_obj, js_implementation_proto_funcs, countof(js_implementation_proto_funcs));
//	implementation_class = JS_NewCFunction2(ctx, js_implementation_ctor, "implementation", 0, JS_CFUNC_constructor, 0);
//	JS_SetConstructor(ctx, implementation_class, implementation_obj);
	JS_SetClassProto(ctx, js_implementation_class_id, implementation_obj);

	RETURN_JS(implementation_obj);
}
