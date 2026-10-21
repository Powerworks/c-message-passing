#include "actor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_ACTORS 64

static Actor *g_actors[MAX_ACTORS];
static size_t  g_actor_count = 0;
static size_t  g_enqueue_seq = 0;
static volatile bool g_stop_requested = false;

static int mailbox_compare(const void *a, const void *b) {
    const Message *ma = *(const Message *const *)a;
    const Message *mb = *(const Message *const *)b;
    if (ma->priority != mb->priority) return mb->priority - ma->priority;
    return (int)(ma->enqueue_seq - mb->enqueue_seq);
}

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
    copy->enqueue_seq = g_enqueue_seq++;
    if (msg->payload_size > 0) {
        copy->payload = malloc(msg->payload_size);
        if (!copy->payload) { free(copy); return false; }
        memcpy(copy->payload, msg->payload, msg->payload_size);
    } else {
        copy->payload = NULL;
    }
    a->mailbox[a->mailbox_len++] = copy;
    qsort(a->mailbox, a->mailbox_len, sizeof(Message *), mailbox_compare);
    return true;
}

static bool mailbox_pop(Actor *a, Message *out) {
    if (a->mailbox_len == 0) return false;
    Message *front = a->mailbox[0];
    *out = *front;
    memmove(a->mailbox, a->mailbox + 1, (a->mailbox_len - 1) * sizeof(Message *));
    a->mailbox_len--;
    free(front);
    return true;
}

Actor *actor_spawn(const char *name, ActorFn fn, void *state) {
    if (g_actor_count >= MAX_ACTORS) return NULL;
    Actor *a = calloc(1, sizeof(Actor));
    if (!a) return NULL;
    a->fn = fn;
    a->state = state;
    a->max_restarts = 3;
    if (name) {
        strncpy(a->name, name, sizeof(a->name) - 1);
        a->name[sizeof(a->name) - 1] = '\0';
        a->named = true;
    }
    g_actors[g_actor_count++] = a;
    return a;
}

Actor *actor_lookup(const char *name) {
    if (!name) return NULL;
    for (size_t i = 0; i < g_actor_count; i++)
        if (g_actors[i]->named && strcmp(g_actors[i]->name, name) == 0)
            return g_actors[i];
    return NULL;
}

void actor_set_supervisor(Actor *a, SupervisorFn fn, void *ctx) {
    if (a) { a->on_crash = fn; a->supervisor_ctx = ctx; }
}

void actor_set_max_restarts(Actor *a, int max) {
    if (a) a->max_restarts = max;
}

void actor_mark_crashed(Actor *a, const void *payload, size_t payload_size) {
    if (!a) return;
    a->crashed = true;
    if (payload && payload_size > 0) {
        free(a->restart_payload);
        a->restart_payload = malloc(payload_size);
        if (a->restart_payload) {
            memcpy(a->restart_payload, payload, payload_size);
            a->restart_payload_size = payload_size;
        }
    }
}

void actor_send(Actor *dest, int type, const void *payload, size_t payload_size, Actor *sender, int priority) {
    Message msg = { .type = type, .payload = (void *)payload, .payload_size = payload_size,
                    .sender = sender, .priority = priority };
    (void)mailbox_push(dest, &msg);
}

bool actor_receive(Actor *self, Message *out) {
    return mailbox_pop(self, out);
}

void actor_scheduler_stop(void) { g_stop_requested = true; }

size_t actor_count_live(void) {
    size_t n = 0;
    for (size_t i = 0; i < g_actor_count; i++)
        if (!g_actors[i]->stopped && !g_actors[i]->crashed) n++;
    return n;
}

static void restart_actor(Actor *a) {
    if (!a) return;
    if (a->max_restarts != -1 && a->restart_count >= a->max_restarts) {
        a->stopped = true;
        a->crashed = false;
        printf("[%s] max restarts reached, stopping\n", a->name);
        return;
    }
    a->restart_count++;
    a->crashed = false;
    a->stopped = false;
    printf("[%s] restart #%d\n", a->name, a->restart_count);
    if (a->restart_payload && a->restart_payload_size > 0) {
        Message m = { .type = 0, .payload = a->restart_payload, .payload_size = a->restart_payload_size,
                      .sender = NULL, .priority = 5 };
        mailbox_push(a, &m);
    }
    if (a->on_crash) a->on_crash(a, a, a->supervisor_ctx);
}

void actor_scheduler_run(void) {
    while (!g_stop_requested && actor_count_live() > 0) {
        for (size_t i = 0; i < g_actor_count && !g_stop_requested; i++) {
            Actor *a = g_actors[i];
            if (a->stopped && !a->crashed) continue;
            if (a->crashed) { restart_actor(a); continue; }

            Message msg;
            if (actor_receive(a, &msg)) {
                a->fn(a, &msg, a->state);
                if (a->crashed) { restart_actor(a); continue; }
                if (a->stopped) continue;
            }
        }
    }
    /* Final crash pass: restart any still-crashing actors once */
    for (size_t i = 0; i < g_actor_count; i++) {
        Actor *a = g_actors[i];
        if (a->crashed) restart_actor(a);
    }
}

void actor_shutdown_all(void) {
    for (size_t i = 0; i < g_actor_count; i++) {
        Actor *a = g_actors[i];
        while (a->mailbox_len > 0) {
            Message m;
            if (mailbox_pop(a, &m)) free(m.payload);
        }
        free(a->restart_payload);
        free(a->mailbox);
        free(a);
    }
    g_actor_count = 0;
    g_enqueue_seq = 0;
}
