#include "command_parser.hpp"
#include "db_interface.hpp"

#include <exception>
#include <iostream>
#include <string>

using namespace std;

int main() {
    try {
        DatabaseInterface database;
        string line;

        while (true) {
            cout << "db> ";
            if (!getline(cin, line)) {
                database.shutdown();
                break;
            }

            string error;
            auto parsed = parse_command(line, error);
            if (!parsed.has_value()) {
                cout << "ERROR: " << error << '\n';
                continue;
            }

            switch (parsed->type) {
                case ParsedCommand::Type::Insert:
                    database.insert(parsed->first, parsed->text);
                    cout << "OK\n";
                    break;
                case ParsedCommand::Type::Get: {
                    auto value = database.get(parsed->first);
                    if (value.has_value()) {
                        cout << value.value() << '\n';
                    } else {
                        cout << "NOT FOUND\n";
                    }
                    break;
                }
                case ParsedCommand::Type::Range: {
                    const auto results = database.range(parsed->first, parsed->second);
                    if (results.empty()) {
                        cout << "EMPTY\n";
                    } else {
                        for (const auto& item : results) {
                            cout << item.first << " => " << item.second << '\n';
                        }
                    }
                    break;
                }
                case ParsedCommand::Type::Flush:
                    database.flush();
                    cout << "OK\n";
                    break;
                case ParsedCommand::Type::Exit:
                    database.shutdown();
                    cout << "BYE\n";
                    return 0;
            }
        }
    } catch (const exception& ex) {
        cerr << "FATAL: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
