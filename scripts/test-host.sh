#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out=$(mktemp -d /tmp/cardtft-tests.XXXXXX)
trap 'rm -rf "$out"' EXIT
flags=(-g -fsanitize=address,undefined -fno-omit-frame-pointer)
for file in components/libvterm/src/*.c; do
    cc "${flags[@]}" -std=c99 -Icomponents/libvterm/include -Icomponents/libvterm/src -c "$file" -o "$out/$(basename "$file" .c).o"
done
c++ "${flags[@]}" -std=c++17 -Imain -Icomponents/libvterm/include tests/terminal_test.cpp main/terminal/terminal_emulator.cpp "$out/"*.o -o "$out/terminal"
"$out/terminal"
c++ "${flags[@]}" -std=c++17 -Imain tests/host_key_policy_test.cpp -o "$out/host-key"
"$out/host-key"
c++ "${flags[@]}" -std=c++17 -ffunction-sections -fdata-sections -Itests/stubs -Imain -Imanaged_components/skuodi__libssh2_esp/libssh2/include tests/ssh_gate_test.cpp main/ssh/ssh_client.cpp main/ssh/host_key_store.cpp -Wl,--gc-sections -Wl,--wrap=connect -lcrypto -o "$out/ssh-gate"
"$out/ssh-gate"
cc "${flags[@]}" -ffunction-sections -fdata-sections -Imanaged_components/m5stack__m5gfx/src -c managed_components/m5stack__m5gfx/src/lgfx/Fonts/efont/lgfx_efont_ja.c -o "$out/efont.o"
c++ "${flags[@]}" -std=c++17 -Imain -Imanaged_components/m5stack__m5gfx/src tests/font_test.cpp "$out/efont.o" -Wl,--gc-sections -o "$out/font"
"$out/font"
c++ "${flags[@]}" -std=c++17 -ffunction-sections -fdata-sections -Itests/stubs -Imain -Imanaged_components/skuodi__libssh2_esp/libssh2/include tests/transfer_test.cpp main/ssh/ssh_client.cpp main/ssh/host_key_store.cpp -Wl,--gc-sections -Wl,--wrap=connect -lcrypto -o "$out/transfer"
"$out/transfer"
