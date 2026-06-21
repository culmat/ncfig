module;

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

export module nctui.cmdline;

// Source - https://stackoverflow.com/a/868894
// Posted by iain, modified by community. See post 'Timeline' for change history
// Retrieved 2026-06-18, License - CC BY-SA 3.0

import nctui.tweening;

/**
 * @file cmdline.cppm
 * @brief Command line argument parsing module for ncfig.
 *
 * This module provides the CmdLine class for parsing command-line arguments
 * and options, with special support for gradient color parsing.
 */

/**
 * @class CmdLine
 * @brief Parses and manages command-line arguments and options.
 *
 * The CmdLine class provides functionality to parse command-line arguments,
 * extract option values, check for option existence, and parse gradient color
 * specifications from command-line options.
 */
export class CmdLine
{
public:
    /**
     * @brief Constructs a CmdLine object and parses command-line arguments.
     *
     * @param argc Reference to the argument count from main().
     * @param argv The argument vector from main().
     *
     * The constructor stores all command-line arguments (skipping argv[0]) and
     * immediately parses them into options and positional arguments.
     */
    CmdLine(int& argc, char** argv)
    {
        for (int i = 1; i < argc; ++i) {
            this->tokens.push_back(std::string(argv[i]));
        }

        parse();
    }

    /**
     * @brief Gets the value of a command-line option.
     *
     * @param option The option name to look up (e.g., "-v", "--verbose").
     * @return The value associated with the option, or an empty string if not found.
     *
     * For options without values (flags), returns an empty string.
     */
    const std::string& getCmdOption(const std::string &option) const
    {
        auto it = option_values.find(option);

        if (it != option_values.end()) {
            return it->second;
        }

        static const std::string empty_string("");
        return empty_string;
    }

    /**
     * @brief Checks if a command-line option exists.
     *
     * @param option The option name to check (e.g., "-v", "--verbose").
     * @return true if the option was specified on the command line, false otherwise.
     */
    bool cmdOptionExists(const std::string &option) const
    {
        return option_values.find(option) != option_values.end();
    }

    /**
     * @brief Gets the positional argument (text to display).
     *
     * @return The last non-option argument, or empty string if none found.
     */
    const std::string& getText() const
    {
        return text;
    }

    /**
     * @brief Parses gradient options and returns configured Gradient structs.
     *
     * @param vgradient_str Value from -v option (vertical gradient)
     * @param hgradient_str Value from -h option (horizontal gradient)
     * @param rgradient_str Value from -r option (radial gradient)
     * @return A pair of (first, second) Gradient structs ready for use with FigletFont::render()
     */
    static std::pair<Tweening::Gradient, Tweening::Gradient> parseGradients(
        const std::string& vgradient_str,
        const std::string& hgradient_str,
        const std::string& rgradient_str)
    {
        Tweening::Gradient first{};
        Tweening::Gradient second{};

        // Helper lambda to parse a gradient sparseGradientStringtring into start, center, end
        auto parseGradientString = [](
                                       const std::string & gradient_str,
                                       Tweening::Gradient & grad,
                                       Tweening::GradientType type,
        Tweening::Orientation orient = Tweening::Orientation::Unconfigured) {
            if (gradient_str.empty()) {
                return;
            }

            // Split by ':'
            std::vector<std::string> parts;
            std::size_t start = 0;
            std::size_t pos;

            while ((pos = gradient_str.find(":", start)) != std::string::npos) {
                parts.push_back(gradient_str.substr(start, pos - start));
                start = pos + 1;
            }

            parts.push_back(gradient_str.substr(start));

            if (parts.size() < 2) {
                throw std::invalid_argument("Gradient must have at least 2 colors separated by ':'");
            }

            // First always goes to start, last always goes to end
            grad.start = std::stoul(parts[0], nullptr, 16);
            grad.end = std::stoul(parts.back(), nullptr, 16);

            // If there are exactly 3 parts, the middle one goes to center
            if (parts.size() == 3) {
                grad.center = std::stoul(parts[1], nullptr, 16);
            } else {
                grad.center = 0; // No center color
            }

            grad.type = type;
            grad.orientation = orient;
        };

        parseGradientString(
            vgradient_str,
            first,
            Tweening::GradientType::Linear,
            Tweening::Orientation::Vertical);

        if (!hgradient_str.empty()) {
            // when both v and h gradients are specified, use second for horizontal
            Tweening::Gradient &dest = vgradient_str.empty() ? first : second;
            parseGradientString(
                hgradient_str,
                dest,
                Tweening::GradientType::Linear,
                Tweening::Orientation::Horizontal);
        }

        parseGradientString(rgradient_str, first, Tweening::GradientType::Radial);

        return {first, second};
    }

    /**
     * @brief Parses gradient options from the command line and returns configured Gradient structs.
     *
     * Uses getCmdOption to fetch -v, -h, -r values.
     *
     * @return A pair of (first, second) Gradient structs ready for use with FigletFont::render()
     */
    std::pair<Tweening::Gradient, Tweening::Gradient> getGradients() const
    {
        return parseGradients(
                   getCmdOption("-v"),
                   getCmdOption("-h"),
                   getCmdOption("-r")
               );
    }

    /**
     * @brief Validates that radial gradients are not combined with other gradients.
     *
     * @return true if the gradient combination is valid, false otherwise.
     */
    bool validateGradients() const
    {
        std::string rgradient = getCmdOption("-r");
        std::string vgradient = getCmdOption("-v");
        std::string hgradient = getCmdOption("-h");

        return rgradient.empty() || (vgradient.empty() && hgradient.empty());
    }

private:
    /**
     * @brief Parses the stored tokens into options and positional arguments.
     *
     * This method processes the tokens vector and populates the option_values map
     * and text member variable. Options are identified by a leading '-' character.
     * Option values are the following non-option tokens. All non-option tokens
     * are collected into the text member as a space-separated string.
     */
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

    std::vector<std::string> tokens; ///< Storage for all command-line argument tokens.
    std::unordered_map<std::string, std::string> option_values; ///< Map of option names to their values.
    std::string text;                ///< Collected positional arguments as a single string.
};
