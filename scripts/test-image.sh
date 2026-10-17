#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out=$(mktemp -d /tmp/cardtft-image.XXXXXX)
trap 'rm -rf "$out"' EXIT
python3 - "$out/input.png" <<'PY'
import sys,struct,zlib
chunk=lambda t,d: struct.pack('>I',len(d))+t+d+struct.pack('>I',zlib.crc32(t+d))
open(sys.argv[1],'wb').write(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',320,240,8,2,0,0,0))+chunk(b'IDAT',zlib.compress((b'\0'+b'\x20\x80\xe0'*320)*240))+chunk(b'IEND',b''))
PY
for source in components/pngdec/src/*.c; do
cc -g -fsanitize=address,undefined -D__LINUX__ -DPNG_SPLIT_WORKSPACE -Icomponents/pngdec/src -c "$source" -o "$out/$(basename "$source").o"
done
c++ -g -fsanitize=address,undefined -fno-omit-frame-pointer -std=c++17 -D__LINUX__ -DPNG_SPLIT_WORKSPACE -DPNG_MAX_BUFFERED_PIXELS=2592 -Imain -Icomponents/pngdec/src tests/image_test.cpp components/pngdec/src/PNGdec.cpp "$out/"*.o -o "$out/test"
"$out/test" "$out/input.png"
