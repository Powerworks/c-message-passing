# Compounding series

This is **repo #1** in a sequence where each project extracts architecture bits and feeds the next.

## Sequence

| # | Repo | Language/Stack | Core idea extracted |
|---|------|----------------|---------------------|
| 1 | [`c-message-passing`](https://github.com/Powerworks/c-message-passing) | C99 | Actor message passing, mailbox isolation |
| 2 | `ada-contract-state-machine` | Ada | Design-by-contract, protected objects |
| 3 | `soa-contract-first` | HTTP/JSON | Service contracts as bounded boundaries |
| 4 | `elixir-supervision-pool` | Elixir/OTP | Supervision trees, let-it-crash |
| 5 | `swift-notes-offline-first` | Swift | Value types, actors, offline-first UI |
| 6 | `kotlin-tasks-multiplatform` | Kotlin | Structured concurrency, repository protocol |
| 7 | `wasm-actor-runtime` | C → WASM | Sandboxed message passing at the bytecode level |
| 8 | `ebpf-observability` | C + eBPF | Programmable kernelspace tracing & networking |
| 9 | `http3-message-gateway` | C / HTTP/3 | QUIC-based message transport, datagram-first semantics |

## What this repo contributes downstream

- **To Ada:** Message schemas → contract preconditions/postconditions
- **To SOA:** Mailbox as service boundary → autonomous services with explicit contracts
- **To Elixir:** Actor isolation → OTP processes with supervision
- **To Swift/Kotlin:** No shared mutable state → value types and actor-isolated model layers
- **To WASM:** C actors compile to WASM modules; mailboxes become WASM linear-memory boundaries
- **To eBPF:** Actor send/receive maps to eBPF map-based communication between kernelspace and userspace
- **To HTTP/3:** Mailbox send/receive → QUIC datagrams; actor isolation → per-connection stream isolation

## Obsidian links

[[Programming/C|c-message-passing]]  
[[Programming/Ada|ada-contract-state-machine (planned)]]  
[[Programming/Elixir|elixir-supervision-pool (planned)]]  
[[Programming/WASM|wasm-actor-runtime (planned)]]  
[[Programming/eBPF|ebpf-observability (planned)]]  
[[Programming/HTTP3|http3-message-gateway (planned)]]
