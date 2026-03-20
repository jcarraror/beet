#!/usr/bin/env sh
set -eu

MODE="${1:-}"

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"

had_format_changes=0

say() {
    printf '%s\n' "$*"
}

have_cmd() {
    command -v "$1" >/dev/null 2>&1
}

tracked_files() {
    git ls-files
}

format_file() {
    file="$1"

    case "$file" in
        *.c|*.h)
            if have_cmd clang-format; then
                clang-format -i "$file"
            fi
            ;;
        *)
            ;;
    esac
}

format_project() {
    if ! have_cmd clang-format; then
        say "[beet] clang-format not found; skipping auto-format"
        return 0
    fi

    say "[beet] formatting project files"

    for file in $(tracked_files); do
        if [ ! -f "$file" ]; then
            continue
        fi

        case "$file" in
            *.c|*.h)
                before="$(git hash-object "$file" 2>/dev/null || true)"
                format_file "$file"
                after="$(git hash-object "$file" 2>/dev/null || true)"

                if [ "$before" != "$after" ]; then
                    had_format_changes=1
                    say "[beet] formatted: $file"
                fi
                ;;
        esac
    done
}

configure_build_test() {
    say "[beet] configuring"
    cmake -S . -B build

    say "[beet] building"
    cmake --build build

    say "[beet] testing"
    ctest --test-dir build --output-on-failure
}

format_project
configure_build_test

if [ "${MODE}" = "--hook" ] && [ "$had_format_changes" -eq 1 ]; then
    say
    say "[beet] some files were auto-formatted"
    say "[beet] review the changes, stage them, and run git commit again"
    exit 1
fi

say "[beet] checks passed"