Beet full commit map
Phase 0 — Project foundation
✅ build: initialize repo and CMake targets
✅ feat(diag): add source spans and colored diagnostics
✅ docs: add architecture overview and design decision log
✅ feat(source): add owned file loading and line index
Phase 1 — Language shape
✅ docs: add Beet language specification and design principles
✅ refactor(lexer): align token set with Beet syntax
✅ test(lexer): add Beet syntax token coverage
Phase 2 — Parser basics
✅ feat(parser): parse bindings
✅ feat(parser): parse function declarations
✅ feat(parser): parse type declarations and structure fields
Phase 3 — Early semantics
✅ feat(resolve): add symbol table and scope resolution for bindings
✅ chore(dev): add pre-commit quality hook
✅ feat(types): add primitive type nodes and explicit annotation checking
Phase 4 — IR and execution spine
✅ feat(mir): add minimal function MIR skeleton
✅ feat(codegen): emit bytecode for integer literals and returns
✅ feat(vm): add minimal interpreter for integer bytecode
✅ feat(driver): wire compiler pipeline end-to-end for trivial Beet programs
✅ feat(parser): parse explicit return statements into AST
✅ feat(mir): lower return statements from AST
Near-term roadmap
Phase 5 — Real function bodies
✅ feat(parser): parse binding statements inside function bodies
🔜 test(parser): add function body statement coverage
🔜 feat(mir): lower binding statements inside function bodies
🔜 test(mir): add mixed binding and return lowering coverage
Phase 6 — Name-aware execution
🔜 feat(resolve): resolve names used in return expressions
🔜 test(resolve): add local use resolution coverage
🔜 feat(codegen): emit bytecode for returning bound locals from lowered functions
🔜 test(exec): run functions that bind then return locals
Phase 7 — Expression core
🔜 feat(parser): parse unary and binary integer expressions
🔜 test(parser): add expression precedence coverage
🔜 feat(types): type-check integer expressions
🔜 feat(mir): lower arithmetic expressions
🔜 feat(codegen): emit integer arithmetic bytecode
🔜 feat(vm): execute integer arithmetic bytecode
🔜 test(exec): run arithmetic Beet programs end-to-end
Phase 8 — Better statements and control flow
🔜 feat(parser): parse if statements
🔜 feat(types): type-check if conditions
🔜 feat(mir): introduce basic blocks and conditional branches
🔜 feat(codegen): emit branch bytecode
🔜 feat(vm): execute branch bytecode
🔜 test(exec): run conditional Beet programs
🌱 feat(parser): parse while statements
🌱 feat(mir): lower loops
🌱 feat(codegen): emit loop/branch bytecode
🌱 feat(vm): execute loops
🌱 test(exec): run looping Beet programs
Mid-term compiler roadmap
Phase 9 — Real type declarations
🌱 feat(parser): parse choice types
🌱 test(parser): add choice type coverage
🌱 feat(types): type-check structure declarations
🌱 feat(types): type-check choice declarations
🌱 feat(resolve): register user-defined type names
🌱 test(types): validate named type lookup
Phase 10 — Construction and field access
🌱 feat(parser): parse structure construction expressions
🌱 feat(parser): parse field access expressions
🌱 feat(types): type-check structure construction
🌱 feat(types): type-check field access
🌱 feat(mir): lower structure construction and field loads
🌱 feat(codegen): emit aggregate bytecode
🌱 feat(vm): represent simple aggregate values
🌱 test(exec): run structure construction and field access programs
Phase 11 — Function calls
🌱 feat(parser): parse function call expressions
🌱 feat(resolve): resolve called function names
🌱 feat(types): type-check call arguments and returns
🌱 feat(mir): lower function calls
🌱 feat(codegen): emit call bytecode
🌱 feat(vm): add call frames and function dispatch
🌱 test(exec): run multi-function Beet programs
Language usefulness milestone
Phase 12 — Modules and multiple files
🌱 feat(parser): parse module declarations
🌱 feat(driver): compile multiple source files
🌱 feat(resolve): add module-level symbol tables
🌱 feat(resolve): resolve cross-file references
🌱 test(driver): run multi-file Beet programs
Phase 13 — Strings and file IO bridge
🌱 feat(lexer): add string literal tokens
🌱 feat(parser): parse string literals
🌱 feat(types): add String primitive/runtime type
🌱 feat(vm): represent strings
🌱 feat(runtime): add builtin print_int
🌱 feat(runtime): add builtin print_string
🌱 feat(runtime): add builtin read_file
🌱 test(exec): run programs using strings and basic IO

This is a major threshold. After this, Beet becomes useful for compiler-like code.

Ownership path
Phase 14 — Ownership syntax and semantics core
🌱 feat(parser): parse borrowed and owned type annotations
🌱 feat(types): represent ownership-qualified types
🌱 feat(resolve): track local binding mutability and ownership classes
🌱 feat(borrow): add move/use state tracking for locals
🌱 test(borrow): reject use-after-move
🌱 feat(borrow): add shared vs mutable borrow checks
🌱 test(borrow): reject aliasing violations
Phase 15 — Deterministic destruction model
🌱 feat(types): classify trivial and non-trivial value kinds
🌱 feat(mir): insert explicit drop points
🌱 feat(codegen): emit drop bytecode
🌱 feat(vm): execute explicit destruction hooks
🌱 test(exec): validate deterministic destruction order
Compiler maturity path
Phase 16 — Diagnostics become real compiler diagnostics
🌱 feat(diag): attach parser errors to source spans
🌱 feat(diag): attach type errors to source spans
🌱 feat(diag): attach resolve errors to source spans
🌱 feat(diag): add structured notes for common mistakes
Phase 17 — Real AST/HIR split
🌱 feat(hir): add normalized semantic representation
🌱 feat(resolve): lower AST to HIR after resolution
🌱 feat(types): type-check HIR instead of raw AST
🌱 feat(mir): lower HIR to MIR
🌱 test(hir): add normalization coverage

This is where the compiler starts becoming properly maintainable.

Standard library and compiler-in-Beet path
Phase 18 — Minimal standard core
🌱 feat(std): add core module layout
🌱 feat(std): add basic Int and Bool helpers
🌱 feat(std): add String helpers
🌱 feat(std): add simple Result and Option definitions
🌱 test(std): compile and run core examples
Phase 19 — Compiler-support data structures
🌱 feat(std): add dynamic buffer type
🌱 feat(std): add simple array/slice abstractions
🌱 feat(std): add map or symbol-table helper subset
🌱 test(std): validate compiler-support containers

This is the point where Beet can start expressing compiler internals without misery.

Self-hosting path
Phase 20 — Rewrite tiny compiler pieces in Beet
🌱 feat(stage1): write token definitions in Beet
🌱 feat(stage1): write source span helpers in Beet
🌱 feat(stage1): write lexer in Beet
🌱 test(stage1): cross-check C lexer and Beet lexer outputs
Phase 21 — Parser in Beet
🌱 feat(stage1): write parser AST definitions in Beet
🌱 feat(stage1): write binding/function/type parser in Beet
🌱 test(stage1): cross-check C parser and Beet parser outputs
Phase 22 — Frontend in Beet
🌱 feat(stage1): write resolve layer in Beet
🌱 feat(stage1): write primitive type checker in Beet
🌱 feat(stage1): write MIR lowering in Beet
🌱 test(stage1): compile sample Beet programs with Beet frontend
Phase 23 — Stage1 compiler
🌱 feat(stage1): write codegen in Beet
🌱 feat(stage1): add Beet compiler driver
🌱 test(stage1): stage0 compiles stage1 compiler
🌱 test(stage1): stage1 compiles simple Beet programs
Phase 24 — True self-host checkpoint
🌱 feat(stage2): compile Beet compiler with Beet compiler
🌱 test(bootstrap): compare stage1 and stage2 output on sample programs
🌱 test(bootstrap): compile compiler with itself successfully

That is the first real:
Beet compiles Beet

After self-hosting
Phase 25 — Replace stage0 gradually
🌱 chore(stage0): freeze C frontend feature work
🌱 feat(stage1): make Beet compiler default developer path
🌱 test(bootstrap): require self-host round-trip in CI
Phase 26 — Language growth after bootstrap
🌱 feat(parser): parse choice pattern matching
🌱 feat(types): type-check match exhaustiveness
🌱 feat(mir): lower match
🌱 feat(exec): run choice-based control flow
🌱 feat(parser): parse closures
🌱 feat(types): type-check closures
🌱 feat(mir): lower closure environments
🌱 feat(vm): represent closure values
🌱 feat(parser): parse generics
🌱 feat(types): instantiate simple generic functions and types