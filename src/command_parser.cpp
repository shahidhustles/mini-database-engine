#include "command_parser.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

using namespace std;

string trim(const string& text) {
    auto begin = find_if_not(text.begin(), text.end(), [](unsigned char ch) { return isspace(ch) != 0; });
    auto end =
        find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) { return isspace(ch) != 0; }).base();
    if (begin >= end) {
        return "";
    }
    return string(begin, end);
}

string uppercase(string text) {
    transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char ch) { return static_cast<char>(toupper(ch)); });
    return text;
}

optional<ParsedCommand> parse_command(const string& line, string& error) {
    error.clear();

    istringstream input(line);
    string command;
    if (!(input >> command)) {
        error = "Empty input.";
        return nullopt;
    }

    command = uppercase(command);
    ParsedCommand parsed{};

    if (command == "INSERT") {
        parsed.type = ParsedCommand::Type::Insert;
        if (!(input >> parsed.first)) {
            error = "INSERT requires an integer key.";
            return nullopt;
        }

        string remainder;
        getline(input, remainder);
        remainder = trim(remainder);
        if (remainder.empty()) {
            error = "INSERT requires a value.";
            return nullopt;
        }
        if (remainder.size() >= 2 && remainder.front() == '"' && remainder.back() == '"') {
            parsed.text = remainder.substr(1, remainder.size() - 2);
        } else {
            parsed.text = remainder;
        }
        return parsed;
    }

    if (command == "GET") {
        parsed.type = ParsedCommand::Type::Get;
        if (!(input >> parsed.first)) {
            error = "GET requires an integer key.";
            return nullopt;
        }
        return parsed;
    }

    if (command == "RANGE") {
        parsed.type = ParsedCommand::Type::Range;
        if (!(input >> parsed.first >> parsed.second)) {
            error = "RANGE requires two integer keys.";
            return nullopt;
        }
        return parsed;
    }

    if (command == "FLUSH") {
        parsed.type = ParsedCommand::Type::Flush;
        return parsed;
    }

    if (command == "EXIT") {
        parsed.type = ParsedCommand::Type::Exit;
        return parsed;
    }

    error = "Unknown command.";
    return nullopt;
}
