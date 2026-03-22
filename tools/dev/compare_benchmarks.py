#!/usr/bin/env python3
import json
import math
import sys
from pathlib import Path


def load_json(path_str: str) -> dict:
    path = Path(path_str)
    with path.open("r", encoding="ascii") as handle:
        return json.load(handle)


def format_mib(value: float) -> str:
    return f"{value:8.2f} MiB/s"


def format_ms(value: float) -> str:
    return f"{value:7.3f} ms"


def report_one(data: dict, path: str) -> int:
    phases = data["phases"]
    print(f"benchmark: {path}")
    print(f"source bytes: {data['source_bytes']}")
    print(f"target per phase: {data['target_ms']:.1f} ms")
    for phase in phases:
      print(
          f"{phase['name']:<10} "
          f"{phase['iterations']:7d} iters  "
          f"{phase['samples']:3d} samples  "
          f"{format_ms(phase['median_ms'])}  "
          f"{phase['min_ms']:7.3f}..{phase['max_ms']:<7.3f} ms  "
          f"{format_mib(phase['mib_per_s'])}"
      )
    print(f"hotspot: {data['hotspot']}")
    return 0


def phase_map(data: dict) -> dict:
    return {phase["name"]: phase for phase in data["phases"]}


def pct_delta(old: float, new: float) -> float:
    if old == 0.0:
        return math.inf if new > 0.0 else 0.0
    return ((new - old) / old) * 100.0


def format_delta(delta: float) -> str:
    if math.isinf(delta):
        return "+inf%"
    return f"{delta:+6.1f}%"


def report_compare(old_data: dict, new_data: dict, old_path: str, new_path: str) -> int:
    old_phases = phase_map(old_data)
    new_phases = phase_map(new_data)
    shared_names = [name for name in ("parse", "resolve", "type-check") if name in old_phases and name in new_phases]

    print(f"compare: {old_path} -> {new_path}")
    print(f"source bytes: {old_data['source_bytes']} -> {new_data['source_bytes']}")
    print(f"hotspot: {old_data['hotspot']} -> {new_data['hotspot']}")
    for name in shared_names:
        old_phase = old_phases[name]
        new_phase = new_phases[name]
        throughput_delta = pct_delta(old_phase["mib_per_s"], new_phase["mib_per_s"])
        latency_delta = pct_delta(old_phase["median_ms"], new_phase["median_ms"])
        print(
            f"{name:<10} "
            f"{old_phase['mib_per_s']:8.2f} -> {new_phase['mib_per_s']:8.2f} MiB/s  "
            f"{format_delta(throughput_delta)} throughput  "
            f"{format_delta(-latency_delta)} latency improvement"
        )
    print("positive percentages are improvements")
    return 0


def main(argv: list[str]) -> int:
    if len(argv) not in (2, 3):
        print("usage: compare_benchmarks.py <latest.json> [baseline.json]", file=sys.stderr)
        return 1

    if len(argv) == 2:
        return report_one(load_json(argv[1]), argv[1])

    old_path = argv[1]
    new_path = argv[2]
    return report_compare(load_json(old_path), load_json(new_path), old_path, new_path)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
