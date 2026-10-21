# Architecture extract — c-message-passing

**Level:** Principal  
**Purpose:** Extract architecture-relevant bits that compound into the next projects in this series.  
**Format:** Obsidian-friendly markdown with `[[wikilinks]]`.

## Signal (carry forward)

| Pattern | Observation | Why it matters |
|---------|-------------|----------------|
| **Message passing as the primitive** | Actors communicate only via `send`/`receive`; no shared memory between actors. | Eliminates data races structurally. The same abstraction works on one core, many cores, or many machines. |
| **Mailbox isolation** | Each actor owns a private mailbox; messages are deep-copied on send. | The receiver cannot mutate the sender's state. This is the C equivalent of Erlang's process isolation. |
| **Cooperative scheduler** | Round-robin, non-preemptive, single-threaded. | Predictable interleaving; no locks, no condition variables, no priority inversion. |
| **Behaviour-as-function** | An actor is a function invoked with one message at a time. | Stateless between invocations except for explicit state pointer. Easy to test, easy to reason about. |

## Noise (deliberately skipped at this layer)

- **Pthreads / mutexes / condition variables.** The old shared-memory model we are explicitly not using here.
- **Preemptive scheduling.** Adds complexity without adding to the message-passing thesis.
- **Fault tolerance / supervision.** One crashing actor kills the process. That's acceptable for a reference; it's the problem the next project (Elixir/OTP supervision trees) solves.

## Principal takeaway

> Isolation is a property of the runtime boundary, not of programmer discipline. If two actors cannot touch the same memory, you do not need locks to keep them correct.

This is the seed idea we rejected in the 90s because threads were cheaper to implement than message-passing runtimes. With modern hardware and distributed concerns, the trade-off flips.

## Compounds into

- [[../../../../Obsidian/Programming/Frontend/Elixir/|Elixir supervision pool]] — actors with fault isolation and restart strategies
- [[../../../../Obsidian/Programming/SOA/|SOA contracts]] — service boundaries as explicit message contracts
- [[../../../../Ada/|Ada design-by-contract]] — contracts as executable specifications, parallel to message schemas
