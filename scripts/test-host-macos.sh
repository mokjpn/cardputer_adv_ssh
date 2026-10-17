#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out=$(mktemp -d /tmp/cardtft-macos.XXXXXX)
trap 'rm -rf "$out"' EXIT
flags=(-g -fsanitize=address,undefined -fno-omit-frame-pointer)
for file in components/libvterm/src/*.c; do
    cc "${flags[@]}" -std=c99 -Icomponents/libvterm/include -Icomponents/libvterm/src -c "$file" -o "$out/$(basename "$file" .c).o"
done
c++ "${flags[@]}" -std=c++17 -Imain -Icomponents/libvterm/include tests/terminal_test.cpp main/terminal/terminal_emulator.cpp "$out/"*.o -o "$out/terminal"
"$out/terminal"
c++ "${flags[@]}" -std=c++17 -Imain tests/host_key_policy_test.cpp -o "$out/host-key"
"$out/host-key"
# Apple ld has no GNU --wrap. Change only a temporary test copy of the source.
python3 - "$out/ssh_mock.cpp" <<'PY'
from pathlib import Path
import sys
s=Path('main/ssh/ssh_client.cpp').read_text()
call='::connect(socket_fd_, item->ai_addr, item->ai_addrlen)'
assert s.count(call)==1
s=s.replace(call,'::__wrap_connect(socket_fd_, item->ai_addr, item->ai_addrlen)')
Path(sys.argv[1]).write_text('#include <sys/socket.h>\nextern "C" int __wrap_connect(int,const sockaddr*,socklen_t);\n'+s)
PY
for name in ssh_gate transfer; do
    c++ "${flags[@]}" -std=c++17 -ffunction-sections -fdata-sections -Itests/stubs -Imain -Imanaged_components/skuodi__libssh2_esp/libssh2/include -I/opt/homebrew/opt/openssl@3/include "tests/${name}_test.cpp" "$out/ssh_mock.cpp" main/ssh/host_key_store.cpp -Wl,-dead_strip -L/opt/homebrew/opt/openssl@3/lib -lcrypto -o "$out/$name"
    "$out/$name"
done
cc "${flags[@]}" -ffunction-sections -fdata-sections -Imanaged_components/m5stack__m5gfx/src -c managed_components/m5stack__m5gfx/src/lgfx/Fonts/efont/lgfx_efont_ja.c -o "$out/efont-font.o"
c++ "${flags[@]}" -std=c++17 -Imain -Imanaged_components/m5stack__m5gfx/src tests/font_test.cpp "$out/efont-font.o" -Wl,-dead_strip -o "$out/font"
"$out/font"
