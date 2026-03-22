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

should_valgrind_test_exe() {
    case "$1" in
        test_diag_print|test_smoke_compiler|test_smoke_vm)
            return 1
            ;;
        test_*)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
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

run_sanitizer_tests() {
    if [ "${BEET_SKIP_SANITIZERS:-0}" = "1" ]; then
        say "[beet] BEET_SKIP_SANITIZERS=1; skipping sanitizers"
        return 0
    fi

    say "[beet] configuring sanitizer build"
    cmake -S . -B build-sanitize -DBEET_ENABLE_SANITIZERS=ON

    say "[beet] building sanitizer targets"
    cmake --build build-sanitize

    say "[beet] testing under sanitizers"
    ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1:halt_on_error=1}" \
    UBSAN_OPTIONS="${UBSAN_OPTIONS:-halt_on_error=1:print_stacktrace=1}" \
        ctest --test-dir build-sanitize --output-on-failure
}

run_valgrind_tests() {
    if [ "${BEET_SKIP_VALGRIND:-0}" = "1" ]; then
        say "[beet] BEET_SKIP_VALGRIND=1; skipping valgrind"
        return 0
    fi

    if ! have_cmd valgrind; then
        say "[beet] valgrind not found; skipping valgrind tests"
        return 0
    fi

    : > "$tmp_files"
    for test_exe in build/test_*; do
        [ -f "$test_exe" ] || continue
        [ -x "$test_exe" ] || continue

        test_name="$(basename "$test_exe")"
        if should_valgrind_test_exe "$test_name"; then
            printf '%s\n' "$test_exe" >> "$tmp_files"
        fi
    done

    if [ ! -s "$tmp_files" ]; then
        say "[beet] no valgrind-eligible unit test executables found"
        return 0
    fi

    say "[beet] running valgrind on selected unit test executables"

    while IFS= read -r test_exe; do
        [ -n "$test_exe" ] || continue
        say "[beet] valgrind: ${test_exe#./}"
        valgrind \
            --quiet \
            --error-exitcode=1 \
            --leak-check=full \
            --show-leak-kinds=definite,possible \
            --errors-for-leak-kinds=definite,possible \
            --track-origins=yes \
            "$test_exe"
    done < "$tmp_files"
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
run_valgrind_tests
run_sanitizer_tests
say "[beet] checks passed"
