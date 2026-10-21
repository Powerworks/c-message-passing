# System design — C message-passing runtime

## Scope

A single-process, single-threaded actor runtime written in C99. Actors are the unit of isolation; messages are the unit of communication. There is no shared heap between actors, no pthreads, and no locks.

## Components

### Actor (`struct Actor`)

Each actor owns:
- A behaviour function (`ActorFn`)
- Opaque caller-supplied state
- A private mailbox (dynamic array of `Message *`)

Actors are registered in a global array (`g_actors`) at spawn time. The scheduler iterates this array round-robin.

### Mailbox

A per-actor FIFO queue of messages. `actor_send` deep-copies the payload into a heap-allocated `Message` and appends it to the destination's mailbox. `actor_receive` pops the head.

Because the payload is copied on send, the sender and receiver never touch the same memory. This is the core isolation guarantee.

### Scheduler

`actor_scheduler_run()` loops while actors are live and no stop has been requested. Each iteration gives every live actor one turn: pop one message from its mailbox, invoke its behaviour function with that message.

If an actor's mailbox is empty, its function is invoked with `msg == NULL`, allowing actors to perform initial work (e.g., send the first `PING`) on their first tick.

## Message flow (Ping ↔ Pong)

```
ping mailbox              pong mailbox
   |                         |
   |--- send(MSG_PING) ----> |
   |                         |--- pop PING
   |                         |   invoke pong_fn
   |                         |--- send(MSG_PONG) -->
   |--- pop PONG             |
   |   invoke ping_fn        |
   |--- send(MSG_PING) ----> |
   ...                       ...
```

## Memory model

- No actor may hold a pointer into another actor's state.
- Mailbox nodes are freed by the queue after `actor_receive` pops them.
- On shutdown, all remaining messages in all mailboxes are freed.
- Actors signal termination by setting `self->stopped = true` and optionally calling `actor_scheduler_stop()`.

## Concurrency model

Cooperative, single-threaded, non-preemptive. An actor runs to completion of its behaviour function before the scheduler yields to the next actor. This eliminates data races entirely at the cost of true parallelism; it is a deliberate choice for a reference implementation that prioritises isolation semantics over throughput.

## Decisions recorded

See `design/adr/0001-actors-over-threads.md`.

## Diagrams

See `design/diagrams/actor-topology.mmd`.
