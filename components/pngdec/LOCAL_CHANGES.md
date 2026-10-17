# CardTFT local changes (2026-10-17)

Upstream commit and license remain as recorded in UPSTREAM and LICENSE.

`PNG_SPLIT_WORKSPACE` preserves the complete 32,768-byte DEFLATE dictionary but
owns it as four independent 8,192-byte allocations. The PNG object keeps the
inflate state inline; dictionary access in inflate.c/inffast.c maps logical
positions to pages, including circular wrap and window-to-output matches.
The optional mode also handles dictionary export, state copy, and cleanup.
The ordinary monolithic mode remains available when the macro is absent.

The C++ PNG wrapper owns the pages, rejects incomplete allocation, and releases
all allocated pages on destruction. Copying its ownership is disabled.
Tests can inject allocation/free functions with PNGDEC_ALLOC/PNGDEC_FREE.
The segmented mode uses byte-safe copies instead of unaligned uint32_t accesses.

The project line buffer is 2,592 bytes: two 1,281-byte full-width RGBA rows plus
30 bytes of filter/alignment padding. The prior 2,562-byte override overlapped
adjacent working storage for 320-pixel RGBA rows; a full-size RGBA corpus case
reproduced that failure before the padding correction.

There is no dictionary-size restriction, new image-format restriction, terminal
history reduction, or SSH behavior change. Device timing and actual fragmented
allocation must still be verified on hardware.
