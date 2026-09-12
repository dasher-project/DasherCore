#!/usr/bin/env bash
# Exported-symbol freeze for libdasher (todo.md Phase 0.3).
#
# Gates on the C API surface ONLY (dasher_* symbols). Internal C++ symbols
# are NOT frozen: the library exports ~1200 of them by accident (no
# -fvisibility=hidden), and freezing them would trip on every internal
# rename during the refactor — alarm fatigue that defeats the freeze.
#
# Fails if any dasher_* symbol in tests/baseline_symbols.txt is missing from
# the built library (removal/rename = C ABI break). New dasher_* symbols only
# produce a NOTE — review them, then update the baseline deliberately:
#   nm -D --defined-only <lib> | awk '$2=="T"{print $3}' | sed 's/^_//' \
#       | grep '^dasher_' | sort > tests/baseline_symbols.txt
#
# LINUX ONLY as an automated gate. On macOS, nm -D is unreliable for Mach-O
# (leading-underscore normalization is handled, but untested); on Windows
# there is no nm/bash — use dumpbin /exports there.
#
# Usage: tests/check_symbols.sh [path-to-libdasher.so]
set -euo pipefail

LIB="${1:-build/bin/libdasher.so}"
BASELINE="$(cd "$(dirname "$0")" && pwd)/baseline_symbols.txt"

if [ ! -f "$LIB" ]; then
  echo "FAIL: library not found: $LIB (build first, or pass a path)" >&2
  exit 2
fi
if [ ! -f "$BASELINE" ]; then
  echo "FAIL: baseline not found: $BASELINE" >&2
  exit 2
fi

# Current exports: defined text symbols; strip the Mach-O leading
# underscore (Darwin prefixes C symbols with '_'; Linux does not) and keep
# only the C API surface.
NOW="$(mktemp)"
trap 'rm -f "$NOW"' EXIT
# LC_ALL=C: stable collation for sort/comm regardless of CI locale.
LC_ALL=C nm -D --defined-only "$LIB" | awk '$2=="T"{print $3}' | sed 's/^_//' | grep '^dasher_' | LC_ALL=C sort > "$NOW"

REMOVED=$(LC_ALL=C comm -13 "$NOW" "$BASELINE")
ADDED=$(LC_ALL=C comm -23 "$NOW" "$BASELINE")

if [ -n "$REMOVED" ]; then
  echo "FAIL: exported C API symbols removed vs baseline (ABI break):" >&2
  echo "$REMOVED" >&2
  exit 1
fi

if [ -n "$ADDED" ]; then
  echo "NOTE: new exported dasher_* symbols (update baseline if intended):"
  echo "$ADDED"
fi

echo "OK: C API symbols match baseline ($(wc -l < "$BASELINE") symbols)."
