#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
"${CXX:-g++}" -std=c++98 -pedantic-errors -Wall -Wextra -I"$root/include" \
  "$root/tests/rev44_xmm_encoder.cpp" "$root/src/backend/x86_64/InstructionEncoder.cpp" -o "$tmp/encoder"
"$tmp/encoder"
