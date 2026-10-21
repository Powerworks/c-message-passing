# ADR 0001 — Actors over threads

## Status

Accepted

## Context

A C concurrency runtime needs a communication primitive. The conventional choice in the 90s/00s was pthreads + shared memory protected by mutexes. That model treats threads as the unit of execution and memory as shared by default, then layers locking on top to prevent corruption.

We rejected that for this reference because:

1. **Shared mutable state is the source of races.** Locks are a corrective measure, not a structural guarantee.
2. **C has no built-in mailbox or scheduler.** Adopting threads means adopting the entire POSIX concurrency surface (mutexes, condition variables, barriers, TLS), which is heavy for a teaching/reference implementation.
3. **The actor model predates threads in mainstream systems** (Actors, 1973; Erlang, 1986) and was sidelined not because it was wrong, but because hardware and tooling made shared-memory threading cheaper to implement on single-core machines.

## Decision

Use isolated actors with private mailboxes and asynchronous message passing as the sole communication mechanism. Scheduling is cooperative and single-threaded.

## Consequences

### Positive

- **No data races by construction.** Actors never share memory; messages are deep-copied on send.
- **Mental model matches distributed systems.** The same send/receive semantics scale from one process to many nodes.
- **Simple to reason about.** Behaviour functions are invoked with one message at a time; no re-entrancy concerns.

### Negative

- **No true parallelism.** Single-threaded scheduler means actors interleave rather than run simultaneously.
- **Copy cost.** Every message payload is heap-copied; large payloads are expensive.
- **No built-in fault isolation.** If an actor's behaviour function crashes (segfault), the whole process dies. Supervision (as in Erlang/Elixir) is a layer to add later — see the Elixir repo in this series.

## Related

- [[architecture/ARCHITECTURE]] — the principal-level extract of this decision
- [[docs/COMPOUNDING]] — how this feeds the Ada and Elixir projects
