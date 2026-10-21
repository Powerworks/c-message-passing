#ifndef ACTOR_H
#define ACTOR_H

#include <stddef.h>
#include <stdbool.h>

typedef struct Actor Actor;
typedef struct Message Message;

typedef void (*ActorFn)(Actor *self, Message *msg, void *state);

struct Message {
    int             type;
    void           *payload;
    size_t          payload_size;
    Actor          *sender;
    int             priority;   /* 0 = normal, higher = more urgent */
    size_t          enqueue_seq;/* insertion order for FIFO within priority */
};

struct Actor {
    ActorFn    fn;
    void      *state;
    char       name[64];        /* named actors for lookup */
    bool       named;
    Message  **mailbox;         /* priority-ordered dynamic array */
    size_t     mailbox_len;
    size_t     mailbox_cap;
    bool       stopped;
};

Actor *actor_spawn(const char *name, ActorFn fn, void *state);
Actor *actor_lookup(const char *name);
void actor_send(Actor *dest, int type, const void *payload, size_t payload_size, Actor *sender, int priority);
bool actor_receive(Actor *self, Message *out);
void actor_scheduler_run(void);
void actor_scheduler_stop(void);
size_t actor_count_live(void);
void actor_shutdown_all(void);

#endif
