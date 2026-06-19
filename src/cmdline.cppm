module;

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

export module nctui.cmdline;

// Source - https://stackoverflow.com/a/868894
// Posted by iain, modified by community. See post 'Timeline' for change history
// Retrieved 2026-06-18, License - CC BY-SA 3.0

export class CmdLine
{
public:
    CmdLine(int &argc, char **argv)
    {
        for (int i = 1; i < argc; ++i) {
            this->tokens.push_back(std::string(argv[i]));
        }

        parse();
    }

    const std::string& getCmdOption(const std::string &option) const
    {
        auto it = option_values.find(option);

        if (it != option_values.end()) {
            return it->second;
        }

        static const std::string empty_string("");
        return empty_string;
    }

    bool cmdOptionExists(const std::string &option) const
    {
        return option_values.find(option) != option_values.end();
    }

    /**
     * @brief Gets the positional argument (text to display).
     * @return The last non-option argument, or empty string if none found.
     */
    const std::string& getText() const
    {
        return text;
    }

private:
    void parse()
    {
        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto& token = tokens[i];

            // Check if this is an option (starts with '-')
            if (!token.empty() && token[0] == '-') {
                // This is an option flag
                // Check if there's a next token that is NOT an option (i.e., the value)
                if (i + 1 < tokens.size() &&
                        !tokens[i + 1].empty() &&
                        tokens[i + 1][0] != '-') {
                    // Next token is the value for this option
                    option_values[token] = tokens[i + 1];
                    ++i; // Skip the value in next iteration
                } else {
                    // Option without a value (flag)
                    option_values[token] = "";
                }
            } else {
                // This is a positional argument
                // Collect all consecutive positional arguments and join them with spaces
                std::string collected;

                while (i < tokens.size() && (tokens[i].empty() || tokens[i][0] != '-')) {
                    if (!collected.empty()) {
                        collected += " ";
                    }

                    collected += tokens[i];
                    ++i;
                }

                --i; // Adjust for the loop increment
                text = collected;
            }
        }
    }

    std::vector<std::string> tokens;
    std::unordered_map<std::string, std::string> option_values;
    std::string text;
};
