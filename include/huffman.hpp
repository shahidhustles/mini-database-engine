#pragma once

#include "compressed_value.hpp"

#include <string>

class HuffmanCodec {
public:
    CompressedValue compress(const std::string& text) const;
    std::string decompress(const CompressedValue& value) const;
};
