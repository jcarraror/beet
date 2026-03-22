# Beet Benchmarking

This file describes the current benchmark workflow for Beet stage0.

## Purpose

The frontend benchmark exists to answer two questions:

- what is the current frontend hotspot
- did the last commit improve or regress throughput

The benchmark is not a correctness test.
It is a regression-tracking tool for parse, resolve, and type-check throughput.

## Canonical Artifact

The canonical per-run benchmark artifact is:

- `docs/benchmarks/latest.json`

`tools/dev/run_checks.sh` now writes this file automatically.

Treat that JSON file as the current performance snapshot for the working tree.

## Per-Commit Rule

When a commit may affect compiler performance, compare benchmark JSON before and
after the change.

The minimum workflow is:

1. save a baseline snapshot before the change
2. make the change
3. run checks again to generate a new `latest.json`
4. compare baseline vs latest

Example:

```sh
tools/dev/run_benchmarks.sh --save docs/benchmarks/baseline.json
tools/dev/run_checks.sh
tools/dev/run_benchmarks.sh --compare docs/benchmarks/baseline.json
```

If you only want the current JSON:

```sh
tools/dev/run_benchmarks.sh --json
```

## What The Benchmark Measures

The benchmark reports three frontend phases:

- `parse`
- `resolve`
- `type-check`

Each phase reports:

- calibrated iteration count
- sample count
- median latency
- min/max latency spread
- throughput in MiB/s

The reported `hotspot` is the slowest phase by throughput.

## How To Read It

Use it primarily for comparison across commits.

Rules:

- higher `MiB/s` is better
- lower median `ms` is better
- large `min..max` spread means the result is noisy
- the `hotspot` line tells you where to look first

Right now, parse throughput is expected to be the main hotspot.

## Scope And Limits

This benchmark is intentionally narrow.

It does not yet measure:

- MIR lowering
- codegen
- VM execution
- multi-file compilation
- self-hosted compiler workloads

## Escape Hatches

`tools/dev/run_checks.sh` runs the benchmark by default.

To skip it:

```sh
BEET_SKIP_BENCHMARKS=1 tools/dev/run_checks.sh
```

You can also tune the benchmark:

- `BEET_BENCH_TARGET_MS`
- `BEET_BENCH_SAMPLES`
- `BEET_BENCH_PARSE_ITERS`
- `BEET_BENCH_RESOLVE_ITERS`
- `BEET_BENCH_TYPE_ITERS`

Those are useful when gathering more stable numbers for a specific comparison.
