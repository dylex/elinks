/* The Quickjs window object implementation. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elinks.h"

#include "bfu/dialog.h"
#include "cache/cache.h"
#include "cookies/cookies.h"
#include "dialogs/menu.h"
#include "dialogs/status.h"
#include "document/html/frames.h"
#include "document/document.h"
#include "document/forms.h"
#include "document/view.h"
#include "ecmascript/ecmascript.h"
#include "ecmascript/quickjs.h"
#include "ecmascript/quickjs/window.h"
#include "ecmascript/timer.h"
#include "intl/libintl.h"
#include "main/select.h"
#include "osdep/newwin.h"
#include "osdep/sysname.h"
#include "protocol/http/http.h"
#include "protocol/uri.h"
#include "session/history.h"
#include "session/location.h"
#include "session/session.h"
#include "session/task.h"
#include "terminal/tab.h"
#include "terminal/terminal.h"
#include "util/conv.h"
#include "util/memory.h"
#include "util/string.h"
#include "viewer/text/draw.h"
#include "viewer/text/form.h"
#include "viewer/text/link.h"
#include "viewer/text/vs.h"

#define countof(x) (sizeof(x) / sizeof((x)[0]))

static JSClassID js_window_class_id;

static JSValue
try_resolve_frame(struct document_view *doc_view, char *id)
{
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s\n", __FILE__, __FUNCTION__);
#endif
	struct session *ses = doc_view->session;
	struct frame *target;

	assert(ses);
	target = ses_find_frame(ses, id);
	if (!target) return JS_NULL;
	if (target->vs.ecmascript_fragile)
		ecmascript_reset_state(&target->vs);
	if (!target->vs.ecmascript) return JS_NULL;
	return JS_GetGlobalObject(target->vs.ecmascript->backend_data);
}

/* @window_funcs{"open"} */
JSValue
js_window_open(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s\n", __FILE__, __FUNCTION__);
#endif
	struct view_state *vs;
	struct document_view *doc_view;
	struct session *ses;
	const char *frame = NULL;
	char *url, *url2;
	struct uri *uri;
	static time_t ratelimit_start;
	static int ratelimit_count;

	struct ecmascript_interpreter *interpreter = (struct ecmascript_interpreter *)JS_GetContextOpaque(ctx);
	vs = interpreter->vs;
	doc_view = vs->doc_view;
	ses = doc_view->session;

	if (get_opt_bool("ecmascript.block_window_opening", ses)) {
#ifdef CONFIG_LEDS
		set_led_value(ses->status.popup_led, 'P');
#endif
		return JS_UNDEFINED;
	}

	if (argc < 1) return JS_UNDEFINED;

	/* Ratelimit window opening. Recursive window.open() is very nice.
	 * We permit at most 20 tabs in 2 seconds. The ratelimiter is very
	 * rough but shall suffice against the usual cases. */
	if (!ratelimit_start || time(NULL) - ratelimit_start > 2) {
		ratelimit_start = time(NULL);
		ratelimit_count = 0;
	} else {
		ratelimit_count++;
		if (ratelimit_count > 20) {
			return JS_UNDEFINED;
		}
	}

	const char *str, *string;
	size_t len;

	str = JS_ToCStringLen(ctx, &len, argv[0]);

	if (!str) {
		return JS_EXCEPTION;
	}

	url = stracpy(str);
	JS_FreeCString(ctx, str);

	if (!url) {
		return JS_UNDEFINED;
	}
	trim_chars(url, ' ', 0);
	url2 = join_urls(doc_view->document->uri, url);
	mem_free(url);

	if (!url2) {
		return JS_UNDEFINED;
	}

	if (argc > 1) {
		size_t len2;
		frame = JS_ToCStringLen(ctx, &len2, argv[1]);

		if (!frame) {
			mem_free(url2);
			return JS_EXCEPTION;
		}
	}

	/* TODO: Support for window naming and perhaps some window features? */

	uri = get_uri(url2, 0);
	mem_free(url2);
	if (!uri) {
		if (frame) JS_FreeCString(ctx, frame);
		return JS_UNDEFINED;
	}

	JSValue ret;

	if (frame && *frame && c_strcasecmp(frame, "_blank")) {
		struct delayed_open *deo = mem_calloc(1, sizeof(*deo));

		if (deo) {
			deo->ses = ses;
			deo->uri = get_uri_reference(uri);
			deo->target = stracpy(frame);
			register_bottom_half(delayed_goto_uri_frame, deo);
			ret = JS_TRUE;
			goto end;
		}
	}

	if (!get_cmd_opt_bool("no-connect")
	    && !get_cmd_opt_bool("no-home")
	    && !get_cmd_opt_bool("anonymous")
	    && can_open_in_new(ses->tab->term)) {
		open_uri_in_new_window(ses, uri, NULL, ENV_ANY,
				       CACHE_MODE_NORMAL, TASK_NONE);
		ret = JS_TRUE;
	} else {
		/* When opening a new tab, we might get rerendered, losing our
		 * context and triggerring a disaster, so postpone that. */
		struct delayed_open *deo = mem_calloc(1, sizeof(*deo));

		if (deo) {
			deo->ses = ses;
			deo->uri = get_uri_reference(uri);
			register_bottom_half(delayed_open, deo);
			ret = JS_TRUE;
		} else {
			ret = JS_UNDEFINED;
		}
	}

end:
	done_uri(uri);
	if (frame) JS_FreeCString(ctx, frame);

	return ret;
}

/* @window_funcs{"setTimeout"} */
JSValue
js_window_setTimeout(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s\n", __FILE__, __FUNCTION__);
#endif
	struct ecmascript_interpreter *interpreter = (struct ecmascript_interpreter *)JS_GetContextOpaque(ctx);
	int64_t timeout = 0;
	JSValueConst func;

	if (argc != 2) {
		return JS_UNDEFINED;
	}

	if (JS_ToInt64(ctx, &timeout, argv[1])) {
		return JS_EXCEPTION;
	}

	if (timeout <= 0) {
		return JS_UNDEFINED;
	}

	func = argv[0];

	if (JS_IsFunction(ctx, func)) {
		timer_id_T id = ecmascript_set_timeout2q(interpreter, JS_DupValue(ctx, func), timeout);

		return JS_NewInt64(ctx, reinterpret_cast<int64_t>(id));
	}

	if (JS_IsString(func)) {
		const char *code = JS_ToCString(ctx, func);

		if (!code) {
			return JS_EXCEPTION;
		}
		char *code2 = stracpy(code);
		JS_FreeCString(ctx, code);

		if (code2) {
			timer_id_T id = ecmascript_set_timeout(interpreter, code2, timeout);

			return JS_NewInt64(ctx, reinterpret_cast<int64_t>(id));
		}
	}

	return JS_UNDEFINED;
}

/* @window_funcs{"clearTimeout"} */
JSValue
js_window_clearTimeout(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s\n", __FILE__, __FUNCTION__);
#endif
	struct ecmascript_interpreter *interpreter = (struct ecmascript_interpreter *)JS_GetContextOpaque(ctx);

	if (argc != 1) {
		return JS_UNDEFINED;
	}
	int64_t number;

	if (JS_ToInt64(ctx, &number, argv[0])) {
		return JS_UNDEFINED;
	}

	timer_id_T id = reinterpret_cast<timer_id_T>(number);

	if (found_in_map_timer(id) && (id == interpreter->vs->doc_view->document->timeout)) {
		kill_timer(&interpreter->vs->doc_view->document->timeout);
	}

	return JS_UNDEFINED;
}


static JSValue
js_window_get_property_closed(JSContext *ctx, JSValueConst this_val)
{
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s\n", __FILE__, __FUNCTION__);
#endif
	return JS_FALSE;
}

static JSValue
js_window_get_property_parent(JSContext *ctx, JSValueConst this_val)
{
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s\n", __FILE__, __FUNCTION__);
#endif
	/* XXX: It would be nice if the following worked, yes.
	 * The problem is that we get called at the point where
	 * document.frame properties are going to be mostly NULL.
	 * But the problem is deeper because at that time we are
	 * yet building scrn_frames so our parent might not be there
	 * yet (XXX: is this true?). The true solution will be to just
	 * have struct document_view *(document_view.parent). --pasky */
	/* FIXME: So now we alias window.parent to window.top, which is
	 * INCORRECT but works for the most common cases of just two
	 * frames. Better something than nothing. */

	return JS_UNDEFINED;
}

static JSValue
js_window_get_property_self(JSContext *ctx, JSValueConst this_val)
{
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s\n", __FILE__, __FUNCTION__);
#endif
	JSValue r = JS_DupValue(ctx, this_val);
	RETURN_JS(r);
}

static JSValue
js_window_get_property_status(JSContext *ctx, JSValueConst this_val)
{
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s\n", __FILE__, __FUNCTION__);
#endif
	return JS_UNDEFINED;
}

static JSValue
js_window_set_property_status(JSContext *ctx, JSValueConst this_val, JSValue val)
{
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s\n", __FILE__, __FUNCTION__);
#endif
	struct ecmascript_interpreter *interpreter = (struct ecmascript_interpreter *)JS_GetContextOpaque(ctx);
	struct view_state *vs = interpreter->vs;

	if (!vs) {
		return JS_UNDEFINED;
	}
	size_t len;
	const char *str = JS_ToCStringLen(ctx, &len, val);

	if (!str) {
		return JS_EXCEPTION;
	}
	char *text = stracpy(str);
	JS_FreeCString(ctx, str);

	mem_free_set(&vs->doc_view->session->status.window_status, text);
	print_screen_status(vs->doc_view->session);

	return JS_UNDEFINED;
}

static JSValue
js_window_get_property_top(JSContext *ctx, JSValueConst this_val)
{
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s\n", __FILE__, __FUNCTION__);
#endif
	struct document_view *doc_view;
	struct document_view *top_view;
	JSValue newjsframe;
	struct ecmascript_interpreter *interpreter = (struct ecmascript_interpreter *)JS_GetContextOpaque(ctx);
	struct view_state *vs = interpreter->vs;

	if (!vs) {
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s %d\n", __FILE__, __FUNCTION__, __LINE__);
#endif
		return JS_UNDEFINED;
	}
	doc_view = vs->doc_view;
	top_view = doc_view->session->doc_view;

	assert(top_view && top_view->vs);
	if (top_view->vs->ecmascript_fragile)
		ecmascript_reset_state(top_view->vs);
	if (!top_view->vs->ecmascript) {
		return JS_UNDEFINED;
	}
	newjsframe = JS_GetGlobalObject(top_view->vs->ecmascript->backend_data);

	/* Keep this unrolled this way. Will have to check document.domain
	 * JS property. */
	/* Note that this check is perhaps overparanoid. If top windows
	 * is alien but some other child window is not, we should still
	 * let the script walk thru. That'd mean moving the check to
	 * other individual properties in this switch. */
	if (compare_uri(vs->uri, top_view->vs->uri, URI_HOST)) {
		return newjsframe;
	}
		/* else */
		/****X*X*X*** SECURITY VIOLATION! RED ALERT, SHIELDS UP! ***X*X*X****\
		|* (Pasky was apparently looking at the Links2 JS code   .  ___ ^.^ *|
		\* for too long.)                                        `.(,_,)\o/ */
	return JS_UNDEFINED;
}

JSValue
js_window_alert(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s\n", __FILE__, __FUNCTION__);
#endif
	struct ecmascript_interpreter *interpreter = (struct ecmascript_interpreter *)JS_GetContextOpaque(ctx);

	assert(interpreter);
	struct view_state *vs;
	const char *str, *string;
	size_t len;

	vs = interpreter->vs;

	if (argc != 1)
		return JS_UNDEFINED;

	str = JS_ToCStringLen(ctx, &len, argv[0]);

	if (!str) {
		return JS_EXCEPTION;
	}

	string = stracpy(str);
	JS_FreeCString(ctx, str);

	info_box(vs->doc_view->session->tab->term, MSGBOX_FREE_TEXT,
		N_("JavaScript Alert"), ALIGN_CENTER, string);

	return JS_UNDEFINED;
}

static JSValue
js_window_toString(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
#ifdef ECMASCRIPT_DEBUG
	fprintf(stderr, "%s:%s\n", __FILE__, __FUNCTION__);
#endif
	return JS_NewString(ctx, "[window object]");
}

static const JSCFunctionListEntry js_window_proto_funcs[] = {
	JS_CGETSET_DEF("closed", js_window_get_property_closed, nullptr),
	JS_CGETSET_DEF("parent", js_window_get_property_parent, nullptr),
	JS_CGETSET_DEF("self", js_window_get_property_self, nullptr),
	JS_CGETSET_DEF("status", js_window_get_property_status, js_window_set_property_status),
	JS_CGETSET_DEF("top", js_window_get_property_top, nullptr),
	JS_CGETSET_DEF("window", js_window_get_property_self, nullptr),
	JS_CFUNC_DEF("alert", 1, js_window_alert),
	JS_CFUNC_DEF("clearTimeout", 1, js_window_clearTimeout),
	JS_CFUNC_DEF("open", 3, js_window_open),
	JS_CFUNC_DEF("setTimeout", 2, js_window_setTimeout),
	JS_CFUNC_DEF("toString", 0, js_window_toString)
};

static JSClassDef js_window_class = {
	"window",
};

static JSValue
js_window_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)
{
	JSValue obj = JS_UNDEFINED;
	JSValue proto;
	/* using new_target to get the prototype is necessary when the
	 class is extended. */
	proto = JS_GetPropertyStr(ctx, new_target, "prototype");

	if (JS_IsException(proto)) {
		goto fail;
	}
	obj = JS_NewObjectProtoClass(ctx, proto, js_window_class_id);
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
js_window_init(JSContext *ctx)
{
	/* create the window class */
	JS_NewClassID(&js_window_class_id);
	JS_NewClass(JS_GetRuntime(ctx), js_window_class_id, &js_window_class);

	JSValue global_obj = JS_GetGlobalObject(ctx);
	JS_SetPropertyFunctionList(ctx, global_obj, js_window_proto_funcs, countof(js_window_proto_funcs));
	JS_SetPropertyStr(ctx, global_obj, "window", global_obj);

	JS_FreeValue(ctx, global_obj);

	return 0;
}
