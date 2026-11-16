#pragma once

#include <cstddef>
#include <cstdint>

namespace Core {
uint32_t crc32_mpeg2(const uint8_t* data, size_t len);
}
