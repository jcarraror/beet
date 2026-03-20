# Beet Architecture

Beet is a self-hosted programming language:

- static typing
- ownership + borrowing (no GC)
- deterministic destruction
- register-based bytecode VM
- stage0 compiler written in C

## Layers

Compiler → Bytecode → VM

Compiler produces bytecode consumed by a standalone C VM.

## Compiler Pipeline

Source
→ AST
→ HIR
→ Typed HIR
→ MIR
→ Bytecode


## Runtime

- standalone C VM
- no garbage collector
- explicit allocation and destruction

## Goals (so far)

- strong type safety
- predictable semantics
- minimal hidden behavior
- self-hosting compiler