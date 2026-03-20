#!/usr/bin/env sh
set -eu

MODE="${1:-}"

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"

had_format_changes=0
tmp_files="$(mktemp)"
trap 'rm -f "$tmp_files"' EXIT HUP INT TERM

say() {
    printf '%s\n' "$*"
}

have_cmd() {
    command -v "$1" >/dev/null 2>&1
}

format_tracked_files() {
    if ! have_cmd clang-format; then
        say "[beet] clang-format not found; skipping formatting"
        return 0
    fi

    say "[beet] formatting tracked C/C header files"

    git ls-files '*.c' '*.h' > "$tmp_files"

    while IFS= read -r file; do
        [ -n "$file" ] || continue
        [ -f "$file" ] || continue

        before="$(git hash-object "$file")"
        clang-format -i "$file"
        after="$(git hash-object "$file")"

        if [ "$before" != "$after" ]; then
            git add "$file"
            had_format_changes=1
            say "[beet] formatted: $file"
        fi
    done < "$tmp_files"
}

configure_build_test() {
    say "[beet] configuring"
    cmake -S . -B build

    say "[beet] building"
    cmake --build build

    say "[beet] testing"
    ctest --test-dir build --output-on-failure
}

format_tracked_files

if [ "${MODE}" = "--hook" ] && [ "$had_format_changes" -eq 1 ]; then
    say
    say "[beet] formatting changed files"
    say "[beet] changes were re-staged"
    say "[beet] review them and run git commit again"
    exit 1
fi

configure_build_test
say "[beet] checks passed"