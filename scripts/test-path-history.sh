#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
c++ -std=c++17 -g -fsanitize=address,undefined -fno-omit-frame-pointer -I main tests/path_history_test.cpp -o "$work/test"
"$work/test"
