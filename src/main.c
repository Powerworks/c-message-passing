#include "actor.h"
#include <stdio.h>

enum {
    MSG_PING = 1,
    MSG_PONG = 2,
    MSG_STOP = 3
};

typedef struct {
    int exchanges_left;
    Actor *partner;
} PingPongState;

static void ping_fn(Actor *self, Message *msg, void *state) {
    PingPongState *s = state;

    if (!msg) {
        /* First tick: send the initial PING */
        int payload = 42;
        actor_send(s->partner, MSG_PING, &payload, sizeof(payload), self);
        printf("[%s] sent PING (exchanges left: %d)\n", self->name, s->exchanges_left);
        return;
    }

    if (msg->type == MSG_PONG) {
        s->exchanges_left--;
        printf("[%s] received PONG (exchanges left: %d)\n", self->name, s->exchanges_left);

        if (s->exchanges_left <= 0) {
            int stop = 0;
            actor_send(s->partner, MSG_STOP, &stop, sizeof(stop), self);
            actor_send(self, MSG_STOP, &stop, sizeof(stop), self);
            self->stopped = true;
            s->partner->stopped = true;
            printf("[%s] done. Requesting stop.\n", self->name);
            actor_scheduler_stop();
        } else {
            int payload = s->exchanges_left;
            actor_send(s->partner, MSG_PING, &payload, sizeof(payload), self);
            printf("[%s] sent PING (exchanges left: %d)\n", self->name, s->exchanges_left);
        }
    } else if (msg->type == MSG_STOP) {
        self->stopped = true;
    }
}

static void pong_fn(Actor *self, Message *msg, void *state) {
    (void)state;

    if (!msg) return; /* Pong never starts first */

    if (msg->type == MSG_PING) {
        int payload = 7;
        actor_send(msg->sender, MSG_PONG, &payload, sizeof(payload), self);
        printf("[%s] received PING, sent PONG\n", self->name);
    } else if (msg->type == MSG_STOP) {
        self->stopped = true;
    }
}

int main(void) {
    PingPongState ping_state = { .exchanges_left = 5, .partner = NULL };

    Actor *ping = actor_spawn("ping", ping_fn, &ping_state);
    Actor *pong = actor_spawn("pong", pong_fn, NULL);

    if (!ping || !pong) {
        fprintf(stderr, "failed to spawn actors\n");
        return 1;
    }

    ping_state.partner = pong;

    printf("=== C Message Passing Runtime ===\n");
    printf("Actors: %s, %s\n", ping->name, pong->name);
    printf("No shared heap. No locks. Mailboxes only.\n\n");

    actor_scheduler_run();

    printf("\nScheduler stopped. Live actors: %zu\n", actor_count_live());
    actor_shutdown_all();
    return 0;
}
