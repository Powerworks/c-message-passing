#include "actor.h"
#include <stdio.h>
#include <signal.h>

enum {
    MSG_PING = 1,
    MSG_PONG = 2,
    MSG_STOP = 3,
    MSG_CRASH = 4
};

static int g_crash_count = 0;

static void supervisor_fn(Actor *supervisor, Actor *crashed, void *ctx) {
    (void)supervisor; (void)ctx;
    g_crash_count++;
    printf("[supervisor] %s crashed (%d total), restarting\n", crashed->name, g_crash_count);
}

typedef struct {
    int exchanges_left;
    char partner_name[64];
    int crash_on_next_pong;  /* if 1, crash after receiving next PONG */
} PingPongState;

static void ping_fn(Actor *self, Message *msg, void *state) {
    PingPongState *s = state;

    if (!msg) {
        Actor *p = actor_lookup(s->partner_name);
        if (!p) { self->stopped = true; return; }
        int payload = 42;
        actor_send(p, MSG_PING, &payload, sizeof(payload), self, 5);
        printf("[%s] sent PING (exchanges left: %d)\n", self->name, s->exchanges_left);
        return;
    }

    if (msg->type == MSG_PONG) {
        s->exchanges_left--;
        printf("[%s] received PONG (exchanges left: %d)\n", self->name, s->exchanges_left);
        if (s->exchanges_left <= 0) {
            int stop = 0;
            actor_send(s->partner, MSG_STOP, &stop, sizeof(stop), self, 1);
            actor_send(self, MSG_STOP, &stop, sizeof(stop), self, 1);
            self->stopped = true;
            s->partner->stopped = true;
            actor_scheduler_stop();
        } else if (s->crash_on_next_pong && s->exchanges_left == 2) {
            /* Deliberately crash on the next cycle to test supervisor */
            printf("[%s] crashing deliberately\n", self->name);
            actor_mark_crashed(self, &s->exchanges_left, sizeof(s->exchanges_left));
        } else {
            int payload = s->exchanges_left;
            actor_send(s->partner, MSG_PING, &payload, sizeof(payload), self, 5);
            printf("[%s] sent PING (exchanges left: %d)\n", self->name, s->exchanges_left);
        }
    } else if (msg->type == MSG_STOP) {
        self->stopped = true;
    } else if (msg->type == MSG_CRASH) {
        actor_mark_crashed(self, NULL, 0);
    }
}

static void pong_fn(Actor *self, Message *msg, void *state) {
    (void)state;
    if (!msg) return;
    if (msg->type == MSG_PING) {
        int payload = 7;
        actor_send(msg->sender, MSG_PONG, &payload, sizeof(payload), self, msg->priority);
        printf("[%s] received PING (priority %d), sent PONG\n", self->name, msg->priority);
    } else if (msg->type == MSG_STOP) {
        self->stopped = true;
    }
}

int main(void) {
    PingPongState ping_state = { .exchanges_left = 5, .crash_on_next_pong = 1 };
    strncpy(ping_state.partner_name, "pong", sizeof(ping_state.partner_name) - 1);

    Actor *pong = actor_spawn("pong", pong_fn, NULL);
    Actor *ping = actor_spawn("ping", ping_fn, &ping_state);

    actor_set_supervisor(ping, supervisor_fn, NULL);
    actor_set_max_restarts(ping, 3);

    printf("=== C Message Passing (advanced: supervisor + restart) ===\n");
    actor_scheduler_run();
    printf("\nScheduler stopped. Supervisor restarts: %d. Live actors: %zu\n",
           g_crash_count, actor_count_live());
    actor_shutdown_all();
    return 0;
}
