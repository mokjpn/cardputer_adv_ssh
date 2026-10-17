#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
#include "PNGdec.h"
#include "ssh/png_probe.h"
#include "ssh/png_validate.h"
#include "png_file_callbacks.inl"

struct DrawState { PNG* png; int rows = 0; uint32_t hash = 2166136261u; };
static int draw(PNGDRAW* line) {
    auto& state = *static_cast<DrawState*>(line->pUser);
    assert(line->iWidth == 320 && line->y == state.rows);
    uint16_t row[320];
    state.png->getLineAsRGB565(line, row, PNG_RGB565_LITTLE_ENDIAN, 0);
    for (auto pixel : row) {
        state.hash = (state.hash ^ (pixel & 255))*16777619u;
        state.hash = (state.hash ^ (pixel >> 8))*16777619u;
    }
    ++state.rows;
    return 1;
}

int main(int argc, char** argv) {
    assert(argc == 2);
    FILE* file = fopen(argv[1], "rb"); assert(file);
    adv::PngProbe probe;
    assert(adv::probe_png(file, probe));
    assert(probe.bytes == 6835 && probe.crc32 == 0x69efe9a1);
    assert(probe.header[24] == 8 && probe.header[25] == 2);
    const uint8_t end[] = {0,0,0,0,'I','E','N','D',0xae,0x42,0x60,0x82};
    assert(memcmp(probe.tail.data(), end, sizeof(end)) == 0);
    assert(adv::validate_png(file, 320, 240));
    std::vector<uint8_t> bytes(probe.bytes); rewind(file);
    assert(fread(bytes.data(), 1, bytes.size(), file) == bytes.size()); fclose(file);
    for (int mode = 0; mode < 2; ++mode) {
        auto png = std::make_unique<PNG>(); DrawState state{png.get()};
        const int opened = mode == 0 ? png->openRAM(bytes.data(), bytes.size(), draw)
            : png->open(argv[1], png_open, png_close, png_read, png_seek, draw);
        assert(opened == PNG_SUCCESS);
        assert(png->decode(&state, PNG_CHECK_CRC) == PNG_SUCCESS);
        png->close();
        assert(state.rows == 240 && state.hash == 0x8867b06e);
        printf("%s: 240 rows, RGB565 hash %08x PASS\n", mode == 0 ? "RAM" : "firmware FILE", state.hash);
    }
    // Known CRC32 vector; the short final read exercises the tail shift path.
    file = tmpfile(); assert(file);
    assert(fwrite("123456789", 1, 9, file) == 9);
    assert(adv::probe_png(file, probe));
    assert(probe.bytes == 9 && probe.crc32 == 0xcbf43926);
    assert(memcmp(probe.tail.data()+3, "123456789", 9) == 0);
    fclose(file);
    file = tmpfile(); assert(file);
    std::vector<uint8_t> oversized(adv::kPngMaxBytes+1, 0);
    assert(fwrite(oversized.data(), 1, oversized.size(), file) == oversized.size());
    assert(!adv::probe_png(file, probe)); fclose(file);
    puts("bounded file probe / CRC32 / tail PASS");
}
