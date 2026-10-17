#pragma once
#include <cstddef>
extern "C" void* png_test_alloc(size_t bytes);
extern "C" void png_test_free(void* pointer);
