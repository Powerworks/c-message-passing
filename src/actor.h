#ifndef ACTOR_H
#define ACTOR_H

#include <stddef.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct Actor Actor;
typedef struct Message Message;

/* Actor entry point. Receives one message per invocation.
 * self   - the actor being invoked
 * msg    - the message pulled from its mailbox (NULL if mailbox empty)
 * state  - caller-supplied opaque state pointer
 */
typedef void (*ActorFn)(Actor *self, Message *msg, void *state);

/* A message placed in a mailbox. Payload is copied by value so actors
 * never share mutable memory. */
struct Message {
    int             type;          /* message kind, e.g. PING/PONG/STOP */
    void           *payload;       /* copied payload bytes */
    size_t          payload_size;  /* size of the copied payload */
    Actor          *sender;        /* originating actor, for replies */
};

/* An actor: behaviour function, opaque state, private mailbox. */
struct Actor {
    ActorFn    fn;
    void      *state;
    char       name[32];
    Message  **mailbox;   /* dynamic array of queued messages */
    size_t     mailbox_len;
    size_t     mailbox_cap;
    bool       stopped;
};

/* Spawn a new actor. Returns NULL on allocation failure. */
Actor *actor_spawn(const char *name, ActorFn fn, void *state);

/* Place a copy of the message into the destination actor's mailbox.
 * Payload is deep-copied; the caller retains ownership of `payload`. */
void actor_send(Actor *dest, int type, const void *payload, size_t payload_size, Actor *sender);

/* Pop one message from the actor's mailbox. Returns false if empty. */
bool actor_receive(Actor *self, Message *out);

/* Run the cooperative scheduler until every actor is stopped.
 * Each iteration gives every live actor one turn to process one message. */
void actor_scheduler_run(void);

/* Request all actors to stop after their current turn. */
void actor_scheduler_stop(void);

/* Return the number of currently live actors. */
size_t actor_count_live(void);

#endif /* ACTOR_H */
