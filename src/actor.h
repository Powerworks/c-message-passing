#ifndef ACTOR_H
#define ACTOR_H

#include <stddef.h>
#include <stdbool.h>

typedef struct Actor Actor;
typedef struct Message Message;

typedef void (*ActorFn)(Actor *self, Message *msg, void *state);
typedef void (*SupervisorFn)(Actor *supervisor, Actor *crashed, void *ctx);

struct Message {
    int             type;
    void           *payload;
    size_t          payload_size;
    Actor          *sender;
    int             priority;
    size_t          enqueue_seq;
};

struct Actor {
    ActorFn         fn;
    void           *state;
    char            name[64];
    bool            named;
    bool            stopped;
    bool            crashed;            /* set when fn aborts/returns error */
    Message        *restart_payload;    /* payload to redeliver on restart */
    size_t          restart_payload_size;
    int             restart_count;
    int             max_restarts;       /* -1 = unlimited */
    SupervisorFn    on_crash;           /* supervisor callback */
    void           *supervisor_ctx;
    Message       **mailbox;
    size_t          mailbox_len;
    size_t          mailbox_cap;
};

Actor *actor_spawn(const char *name, ActorFn fn, void *state);
Actor *actor_lookup(const char *name);
void actor_send(Actor *dest, int type, const void *payload, size_t payload_size, Actor *sender, int priority);
bool actor_receive(Actor *self, Message *out);
void actor_scheduler_run(void);
void actor_scheduler_stop(void);
size_t actor_count_live(void);
void actor_shutdown_all(void);

/* Supervisor registration: called when an actor crashes */
void actor_set_supervisor(Actor *a, SupervisorFn fn, void *ctx);
void actor_set_max_restarts(Actor *a, int max);
void actor_mark_crashed(Actor *a, const void *payload, size_t payload_size);

#endif
