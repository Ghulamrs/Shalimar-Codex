#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PORT="$ROOT/src/backend/legacy_port"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
for file in "$PORT"/*.cpp; do
  g++ -std=c++98 -pedantic-errors -Wall -Wextra -I"$PORT" -c "$file" -o "$TMP/$(basename "$file" .cpp).o"
done
g++ -std=c++98 -pedantic-errors -Wall -Wextra -I"$PORT" "$ROOT/tests/backend_legacy_port_smoke.cpp" "$PORT"/*.cpp -o "$TMP/smoke"
"$TMP/smoke"
if grep -nE '\b(new|delete|malloc|free)[[:space:]]*(\(|\[|[[:alpha:]_])' "$PORT"/*.cpp "$PORT"/*.h; then
  echo 'Forbidden explicit allocation in migrated backend' >&2
  exit 1
fi
printf 'rev32 strict C++98 compile, four-target smoke, and allocation scan: PASS\n'
