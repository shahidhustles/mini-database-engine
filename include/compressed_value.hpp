#pragma once

#include <cstdint>
#include <iosfwd>
#include <stdexcept>
#include <utility>
#include <vector>

struct CompressedValue {
    std::vector<std::uint8_t> compressed_bytes;
    std::uint64_t bit_count = 0;
    std::vector<std::pair<std::uint8_t, std::uint32_t>> frequencies;

    std::uint32_t metadata_size() const;
    std::uint32_t serialized_size() const;
    void write(std::ostream& out) const;
    static CompressedValue read(std::istream& in);
};
