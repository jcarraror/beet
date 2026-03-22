#!/usr/bin/env sh
set -eu

repo_root="$(git rev-parse --show-toplevel)"
latest_json="$repo_root/docs/benchmarks/latest.json"
save_path=""
compare_path=""
json_only=0
previous_tmp=""

cleanup() {
    if [ -n "$tmp_json" ] && [ -f "$tmp_json" ]; then
        rm -f "$tmp_json"
    fi
    if [ -n "$previous_tmp" ] && [ -f "$previous_tmp" ]; then
        rm -f "$previous_tmp"
    fi
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --json)
            json_only=1
            shift
            ;;
        --save)
            save_path="$2"
            shift 2
            ;;
        --compare)
            compare_path="$2"
            shift 2
            ;;
        *)
            echo "usage: $0 [--json] [--save path] [--compare baseline.json]" >&2
            exit 1
            ;;
    esac
done

cd "$repo_root"

cmake -S . -B build >&2
cmake --build build --target bench_frontend >&2
mkdir -p "$repo_root/docs/benchmarks"
tmp_json="$(mktemp)"
trap cleanup EXIT HUP INT TERM
./build/bench_frontend --json > "$tmp_json"
mv "$tmp_json" "$latest_json"

if [ -n "$save_path" ]; then
    mkdir -p "$(dirname "$save_path")"
    cp "$latest_json" "$save_path"
fi

if [ "$json_only" -eq 1 ]; then
    cat "$latest_json"
elif [ -n "$compare_path" ]; then
    python3 "$repo_root/tools/dev/compare_benchmarks.py" "$compare_path" "$latest_json"
else
    previous_tmp="$(mktemp)"
    if git show HEAD~1:docs/benchmarks/latest.json > "$previous_tmp" 2>/dev/null; then
        python3 "$repo_root/tools/dev/compare_benchmarks.py" "$previous_tmp" "$latest_json"
    else
        python3 "$repo_root/tools/dev/compare_benchmarks.py" "$latest_json"
        echo "note: no previous committed benchmark snapshot to compare against" >&2
    fi
fi
