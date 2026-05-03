#pragma once

#include <optional>
#include <string>

struct ParsedCommand {
    enum class Type { Insert, Get, Range, Flush, Exit };

    Type type;
    int first = 0;
    int second = 0;
    std::string text;
};

std::optional<ParsedCommand> parse_command(const std::string& line, std::string& error);
