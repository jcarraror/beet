# Beet Design Decisions

This file records architectural and language decisions.
Entries are append-only.

---

## 2026-03-20 — No Garbage Collector

Beet does not use a tracing GC.

Instead:
- ownership and borrowing enforce memory safety
- destruction is deterministic

Reason:
- predictable performance
- simpler runtime
- aligns with Rust-like safety goals

Tradeoff:
- more complex compiler (borrow checking)

---

## 2026-03-20 — Register-Based VM

The VM uses a register-based bytecode.

Reason:
- closer to MIR representation
- fewer instructions than stack VM
- easier future optimization

Tradeoff:
- more complex interpreter

---

## 2026-03-20 — Stage0 Compiler in C

Initial compiler is written in C.

Reason:
- zero dependencies
- easy bootstrap
- full control over memory

Plan:
- replace with self-hosted compiler later

---

## 2026-03-20 — Explicit Typing + Local Inference

Function signatures must be explicit.
Local variables may be inferred.

Reason:
- simpler type system
- better diagnostics
- easier implementation

Tradeoff:
- more verbose than full inference systems

---

## 2026-03-20 — Source Files Own Their Text Buffers

Compiler source files are represented by owned buffers rather than borrowed
C strings.

Reason:
- explicit ownership and cleanup
- safe file loading
- predictable lifetime across compiler passes

Additionally, source files maintain a line-start index.

Reason:
- efficient offset to line/column mapping
- avoids repeated O(n) scans in diagnostics and later lexer errors

Tradeoff:
- more memory overhead
- setup work when loading each file