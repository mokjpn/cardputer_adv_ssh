#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out=$(mktemp -d /tmp/cardtft-png-file.XXXXXX)
trap 'rm -rf "$out"' EXIT
flags=(-g -fsanitize=address,undefined -fno-omit-frame-pointer)
python3 - "$out" <<'PY'
from pathlib import Path
import sys
out=Path(sys.argv[1])
source=Path('main/drivers/display.cpp').read_text()
(out/'png_file_callbacks.inl').write_text(source[source.index('void* png_open('):source.index('int png_draw(')])
# The firmware's 32-bit ESP-IDF build does not select ALLOWS_UNALIGNED.
# Use that exact portable branch on the 64-bit host too. Vendor source stays intact.
source=Path('components/pngdec/src/inflate.c').read_text()
assert source.count('#define ALLOWS_UNALIGNED')==1
(out/'inflate-target.c').write_text(source.replace('#define ALLOWS_UNALIGNED', '#undef ALLOWS_UNALIGNED'))
PY
for source in components/pngdec/src/*.c; do
    input="$source"
    if [[ $(basename "$source") == inflate.c ]]; then input="$out/inflate-target.c"; fi
    cc "${flags[@]}" -D__LINUX__ -DPNG_SPLIT_WORKSPACE -Icomponents/pngdec/src -c "$input" -o "$out/$(basename "$source").o"
done
c++ "${flags[@]}" -std=c++17 -D__LINUX__ -DPNG_SPLIT_WORKSPACE -DPNG_MAX_BUFFERED_PIXELS=2592 -Imain -Icomponents/pngdec/src -I"$out" \
    tests/png_file_test.cpp components/pngdec/src/PNGdec.cpp "$out/"*.o -o "$out/test"
UBSAN_OPTIONS=halt_on_error=1 "$out/test" "${1:-tests/fixtures/png-pattern.png}"
