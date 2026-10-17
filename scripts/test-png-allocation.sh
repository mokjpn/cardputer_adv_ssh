#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out=$(mktemp -d /tmp/cardtft-png-alloc.XXXXXX)
trap 'rm -rf "$out"' EXIT
flags=(-g -fsanitize=address,undefined -fno-omit-frame-pointer)
for source in components/pngdec/src/*.c; do
    cc "${flags[@]}" -D__LINUX__ -DPNG_SPLIT_WORKSPACE -Icomponents/pngdec/src -c "$source" -o "$out/$(basename "$source").o"
done
c++ "${flags[@]}" -std=c++17 -D__LINUX__ -DPNG_SPLIT_WORKSPACE -DPNG_MAX_BUFFERED_PIXELS=2592 \
    -DPNGDEC_ALLOC=png_test_alloc -DPNGDEC_FREE=png_test_free -include tests/png_alloc_hook.h \
    -Icomponents/pngdec/src tests/png_alloc_test.cpp components/pngdec/src/PNGdec.cpp "$out/"*.o -o "$out/test"
UBSAN_OPTIONS=halt_on_error=1 "$out/test" tests/fixtures/png-pattern.png
