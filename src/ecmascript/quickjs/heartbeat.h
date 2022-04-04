#ifndef EL__ECMASCRIPT_QUICKJS_HEARTBEAT_H
#define EL__ECMASCRIPT_QUICKJS_HEARTBEAT_H

#include <quickjs/quickjs.h>

struct heartbeat {
        LIST_HEAD(struct heartbeat);

        int ttl; /* Time to live.  This value is assigned when the
                  * script begins execution and is decremented every
                  * second.  When it reaches 0, script execution is
                  * terminated. */

        struct ecmascript_interpreter *interpreter;
};

struct heartbeat *add_heartbeat(struct ecmascript_interpreter *interpreter);
void done_heartbeat(struct heartbeat *hb);
int js_heartbeat_callback(JSRuntime *rt, void *opaque);

#endif
