#include "actor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_ACTORS 64

static Actor *g_actors[MAX_ACTORS];
static size_t  g_actor_count = 0;
static volatile bool g_stop_requested = false;

/* ---------- mailbox helpers ---------- */

static bool mailbox_push(Actor *a, const Message *msg) {
    if (a->mailbox_len == a->mailbox_cap) {
        size_t new_cap = a->mailbox_cap ? a->mailbox_cap * 2 : 8;
        Message **tmp = realloc(a->mailbox, new_cap * sizeof(Message *));
        if (!tmp) return false;
        a->mailbox = tmp;
        a->mailbox_cap = new_cap;
    }

    Message *copy = malloc(sizeof(Message));
    if (!copy) return false;

    *copy = *msg;
    if (msg->payload_size > 0) {
        copy->payload = malloc(msg->payload_size);
        if (!copy->payload) {
            free(copy);
            return false;
        }
        memcpy(copy->payload, msg->payload, msg->payload_size);
    } else {
        copy->payload = NULL;
    }

    a->mailbox[a->mailbox_len++] = copy;
    return true;
}

static bool mailbox_pop(Actor *a, Message *out) {
    if (a->mailbox_len == 0) return false;

    Message *front = a->mailbox[0];
    *out = *front;

    /* shift remaining messages down */
    memmove(a->mailbox, a->mailbox + 1, (a->mailbox_len - 1) * sizeof(Message *));
    a->mailbox_len--;

    free(front);
    return true;
}

/* ---------- actor API ---------- */

Actor *actor_spawn(const char *name, ActorFn fn, void *state) {
    if (g_actor_count >= MAX_ACTORS) return NULL;

    Actor *a = calloc(1, sizeof(Actor));
    if (!a) return NULL;

    a->fn = fn;
    a->state = state;
    strncpy(a->name, name, sizeof(a->name) - 1);
    a->name[sizeof(a->name) - 1] = '\0';

    g_actors[g_actor_count++] = a;
    return a;
}

void actor_send(Actor *dest, int type, const void *payload, size_t payload_size, Actor *sender) {
    Message msg = {
        .type         = type,
        .payload      = (void *)payload,
        .payload_size = payload_size,
        .sender       = sender,
    };
    (void)mailbox_push(dest, &msg); /* best effort; in production we'd handle OOM */
}

bool actor_receive(Actor *self, Message *out) {
    return mailbox_pop(self, out);
}

void actor_scheduler_stop(void) {
    g_stop_requested = true;
}

size_t actor_count_live(void) {
    size_t n = 0;
    for (size_t i = 0; i < g_actor_count; i++) {
        if (!g_actors[i]->stopped) n++;
    }
    return n;
}

void actor_scheduler_run(void) {
    while (!g_stop_requested && actor_count_live() > 0) {
        for (size_t i = 0; i < g_actor_count && !g_stop_requested; i++) {
            Actor *a = g_actors[i];
            if (a->stopped) continue;

            Message msg;
            if (actor_receive(a, &msg)) {
                a->fn(a, &msg, a->state);

                /* If the actor set itself stopped inside its handler, skip it next round */
                if (a->stopped) continue;
            }

            /* Free any leftover message struct fields if the handler didn't consume payload */
            /* (payload ownership stays with the message queue; handler borrows only) */
        }
    }
}

/* ---------- cleanup ---------- */

void actor_shutdown_all(void) {
    for (size_t i = 0; i < g_actor_count; i++) {
        Actor *a = g_actors[i];
        while (a->mailbox_len > 0) {
            Message m;
            if (mailbox_pop(a, &m)) {
                free(m.payload);
            }
        }
        free(a->mailbox);
        free(a);
    }
    g_actor_count = 0;
}
