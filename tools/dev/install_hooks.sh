#!/usr/bin/env sh
set -eu

repo_root="$(git rev-parse --show-toplevel)"
git config core.hooksPath "$repo_root/.githooks"

chmod +x "$repo_root/.githooks/precommit"
chmod +x "$repo_root/tools/dev/run_checks.sh"
chmod +x "$repo_root/tools/dev/install_hooks.sh"

printf '%s\n' "[beet] git hooks path set to .githooks"
printf '%s\n' "[beet] pre-commit hook installed"