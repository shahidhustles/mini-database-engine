#pragma once

#include "compressed_value.hpp"
#include "trace_models.hpp"

#include <string>

class HuffmanCodec {
public:
    CompressedValue compress(const std::string& text) const;
    std::pair<CompressedValue, CompressionTrace> compress_with_trace(const std::string& text) const;
    std::string decompress(const CompressedValue& value) const;
};
