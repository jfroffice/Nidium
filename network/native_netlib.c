#include "native_netlib.h"
#include "common.h"
#include "ape_dns.h"

#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>


ape_global *native_netlib_init()
{
    ape_global *ape;
    struct _fdevent *fdev;

    if ((ape = malloc(sizeof(*ape))) == NULL) return NULL;

#ifndef __WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    fdev = &ape->events;
    fdev->handler = EVENT_UNKNOWN;
    #ifdef USE_EPOLL_HANDLER
    fdev->handler = EVENT_EPOLL;
    #endif
    #ifdef USE_KQUEUE_HANDLER
    fdev->handler = EVENT_KQUEUE;
    #endif
    #ifdef USE_SELECT_HANDLER
	fdev->handler = EVENT_SELECT;
    #endif

    ape->basemem    = APE_BASEMEM;
    ape->is_running = 1;
    ape->timers.ntimers = 0;
    ape->timers.timers  = NULL;

    ape->timersng.head = NULL;
    ape->timersng.last_identifier = 0;
	
    ape_dns_init(ape);
    events_init(ape);
	
    return ape;
}
