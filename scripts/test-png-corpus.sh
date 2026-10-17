#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out=$(mktemp -d /tmp/cardtft-png-corpus.XXXXXX)
trap 'rm -rf "$out"' EXIT
flags=(-g -fsanitize=address,undefined -fno-omit-frame-pointer)
defines=(-DPNG_SPLIT_WORKSPACE)
if [[ ${PNG_TEST_MONOLITHIC:-0} == 1 ]]; then defines=(-UPNG_SPLIT_WORKSPACE); fi
python3 tests/png_corpus.py "$out/corpus"
python3 - "$out" <<'PY'
from pathlib import Path
import sys
s=Path('main/drivers/display.cpp').read_text()
Path(sys.argv[1],'png_file_callbacks.inl').write_text(s[s.index('void* png_open('):s.index('int png_draw(')])
s=Path('components/pngdec/src/inflate.c').read_text()
Path(sys.argv[1],'inflate-portable.c').write_text(s.replace('#define ALLOWS_UNALIGNED','#undef ALLOWS_UNALIGNED'))
PY
for source in components/pngdec/src/*.c; do
    input="$source"
    if [[ $(basename "$source") == inflate.c ]]; then input="$out/inflate-portable.c"; fi
    cc "${flags[@]}" -D__LINUX__ "${defines[@]}" -Icomponents/pngdec/src -c "$input" -o "$out/$(basename "$source").o"
done
c++ "${flags[@]}" -std=c++17 -D__LINUX__ "${defines[@]}" -DPNG_MAX_BUFFERED_PIXELS="${PNG_TEST_PIXELS:-2592}" -Imain -Icomponents/pngdec/src -I"$out" \
    tests/png_corpus_test.cpp components/pngdec/src/PNGdec.cpp "$out/"*.o -o "$out/test"
UBSAN_OPTIONS=halt_on_error=1 "$out/test" "$out/corpus"
if [[ ${PNG_TEST_MONOLITHIC:-0} != 1 ]]; then
    c++ "${flags[@]}" -std=c++17 -D__LINUX__ -DPNG_SPLIT_WORKSPACE -DPNG_MAX_BUFFERED_PIXELS=2592 \
        -Icomponents/pngdec/src tests/png_inflate_test.cpp "$out/"*.o -o "$out/inflate-test"
    UBSAN_OPTIONS=halt_on_error=1 "$out/inflate-test" "$out/corpus"
fi
