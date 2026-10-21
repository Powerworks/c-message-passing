# c-message-passing

A small actor-style message-passing runtime in C.

**Thesis:** We discarded actor-model concurrency in the 80s/90s because C had no standard mailbox or scheduler, so everyone reached for pthreads + shared memory instead. Now that goalposts have moved (more cores, more distributed systems, more need for isolation), we're re-discovering message passing from first principles.

This is **repo #1** in a compounding reference series on `github.com/Powerworks`:

```
C message passing  ──►  Ada contracts  ──►  SOA services  ──►  Elixir actors  ──►  Swift/Kotlin mobile
```

Each repo pairs **implementation** + **system design** + **extracted architecture notes**, and extracts the architecture bits that feed the next project.

## What's inside

| Path | Purpose |
|------|---------|
| `src/` | C99 implementation: actor runtime, mailboxes, cooperative scheduler |
| `design/` | System design narrative, ADRs, Mermaid diagrams |
| `architecture/` | Principal-level architecture extract (Obsidian-friendly) |
| `docs/` | Compounding links to previous/next projects in the series |

## Build

```bash
cmake -S . -B build
cmake --build build
./build/c-message-passing
```

## Demo

The binary spawns two actors, `ping` and `pong`. They exchange `PING`/`PONG` messages through their mailboxes (no shared heap, no locks) until a count limit is reached, then the scheduler stops.

## Design summary

- **Actors** are isolated state machines with a private mailbox.
- **Messages** are immutable value structs copied into the destination mailbox.
- **Scheduling** is cooperative, single-threaded, round-robin — no preemption, no data races by construction.
- **Communication** is strictly `send` / `receive`; there is no shared state between actors.

See `design/README.md` for the full system design and `architecture/ARCHITECTURE.md` for the principal-level extract.
