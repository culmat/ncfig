module;

#include <ncpp/Plane.hh>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <string_view>
#include <sstream>

export module figlet;

export class Tweening {
public:
    struct Gradient {
        std::uint32_t start;
        std::uint32_t end;
        std::size_t steps;
    };

    static std::vector<uint32_t> RGB(Gradient gradient)
    {
        std::vector<uint32_t> result;
        if (gradient.steps == 0) return result;
        if (gradient.steps == 1) {
            result.push_back(gradient.start);
            return result;
        }

        // Extract the RGB components of start and end
        uint8_t r1 = (gradient.start >> 16) & 0xFF;
        uint8_t g1 = (gradient.start >> 8) & 0xFF;
        uint8_t b1 = gradient.start & 0xFF;

        uint8_t r2 = (gradient.end >> 16) & 0xFF;
        uint8_t g2 = (gradient.end >> 8) & 0xFF;
        uint8_t b2 = gradient.end & 0xFF;

        result.reserve(gradient.steps);
        for (uint32_t i = 0; i < gradient.steps; ++i) {
            // Calculate the interpolation factor (0.0 to 1.0)
            float t = static_cast<float>(i) / (gradient.steps - 1);

            // Interpolate each channel
            uint8_t r = static_cast<uint8_t>(r1 + (r2 - r1) * t);
            uint8_t g = static_cast<uint8_t>(g1 + (g2 - g1) * t);
            uint8_t b = static_cast<uint8_t>(b1 + (b2 - b1) * t);

            // Combine the channels into a uint32_t value
            uint32_t color = (r << 16) | (g << 8) | b;
            result.push_back(color);
        }

        return result;
    }
};

export class FigletFont {
public:
    FigletFont() = default;

    /**
     * @brief Loads and parses a standard FIGlet font (.flf) file.
     * @param filename Path to the .flf file.
     * @return true if successfully loaded and parsed, false otherwise.
     */
    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::in);
        if (!file.is_open()) {
            return false;
        }

        std::string header;
        if (!std::getline(file, header)) {
            return false;
        }

        std::istringstream iss(header);
        std::string magic;
        iss >> magic;

        // FIGlet header signatures always start with "flf2a"
        if (magic.size() < 6 || magic.substr(0, 5) != "flf2a") {
            return false;
        }

        // The 6th character indicates the custom hardblank character (often '$')
        hardblank = magic[5];

        if (!(iss >> height >> baseline >> max_length >> old_layout >> comment_lines)) {
            return false;
        }

        // Skip any comment lines at the beginning of the file
        for (int i = 0; i < comment_lines; ++i) {
            std::string dummy;
            if (!std::getline(file, dummy)) {
                return false;
            }
        }

        char endmark = '\0';
        bool endmark_set = false;

        // 1. Parse standard printable ASCII characters (characters 32 to 126)
        for (char32_t code = 32; code <= 126; ++code) {
            std::vector<std::string> glyph_rows;
            for (int r = 0; r < height; ++r) {
                std::string line;
                if (!std::getline(file, line)) {
                    return false;
                }

                // Detect the endmark character from the end of the very first line
                if (!endmark_set && !line.empty()) {
                    endmark = line.back();
                    endmark_set = true;
                }

                // FIGlet lines end with either 1 endmark, or 2 endmarks on the final glyph line
                if (!line.empty() && line.back() == endmark) {
                    line.pop_back();
                }
                if (!line.empty() && line.back() == endmark) {
                    line.pop_back();
                }

                // Replace custom hardblanks with space characters
                for (char& ch : line) {
                    if (ch == hardblank) {
                        ch = ' ';
                    }
                }

                glyph_rows.push_back(line);
            }
            glyphs[code] = std::move(glyph_rows);
        }

        // 2. Parse any extra/extended Unicode characters present in the font file
        std::string code_line;
        while (std::getline(file, code_line)) {
            if (code_line.empty()) continue;

            std::istringstream code_iss(code_line);
            std::string code_str;
            code_iss >> code_str;

            if (code_str.empty()) continue;

            char32_t code = 0;
            try {
                // Handle hex (0x), octal (0), and base-10 indices
                if (code_str.size() > 2 && code_str[0] == '0' && (code_str[1] == 'x' || code_str[1] == 'X')) {
                    code = std::stoul(code_str, nullptr, 16);
                } else if (code_str.size() > 1 && code_str[0] == '0') {
                    code = std::stoul(code_str, nullptr, 8);
                } else {
                    code = std::stoul(code_str, nullptr, 10);
                }
            } catch (...) {
                continue; // Skip invalid blocks or malformed rows
            }

            std::vector<std::string> glyph_rows;
            bool success = true;
            for (int r = 0; r < height; ++r) {
                std::string line;
                if (!std::getline(file, line)) {
                    success = false;
                    break;
                }

                if (!line.empty() && line.back() == endmark) {
                    line.pop_back();
                }
                if (!line.empty() && line.back() == endmark) {
                    line.pop_back();
                }

                for (char& ch : line) {
                    if (ch == hardblank) {
                        ch = ' ';
                    }
                }
                glyph_rows.push_back(line);
            }

            if (!success) break;
            glyphs[code] = std::move(glyph_rows);
        }

        return true;
    }

    /**
     * @brief Renders text onto an ncpp::Plane using a horizontal full-width alignment.
     * @param plane The target Notcurses C++ plane.
     * @param y Top-left Y row coordinate.
     * @param x Top-left X column coordinate.
     * @param text String view containing UTF-8 input text.
     * @param cstart Starting color for the gradient.
     * @param cend Ending color for the gradient.
     */
    void render(ncpp::Plane* plane, int y, int x, std::string_view text, uint32_t cstart=0, uint32_t cend=0) const {
        if (height <= 0) return;

        auto lines = renderToStrings(text);
        std::vector<uint32_t> colors;

        if (cstart && cend)
            colors = Tweening::RGB(Tweening::Gradient{cstart, cend, lines.size()});

        for (int i = 0; i < lines.size(); ++i) {
            if (cstart && cend)
                plane->set_fg_rgb(colors[i]);

            // Write strings directly onto the target plane coordinates
            plane->putstr(y + i, x, lines[i].c_str());
        }
    }

    /**
     * @brief Renders text onto an ncpp::Plane using a horizontal full-width alignment.
     * @param plane The target Notcurses C++ plane.
     * @param text String view containing UTF-8 input text.
     * @param cstart Starting color for the gradient.
     * @param cend Ending color for the gradient.
     */
    void render(ncpp::Plane* plane, std::string_view text, uint32_t cstart=0, uint32_t cend=0) const {
        if (height <= 0) return;

        auto lines = renderToStrings(text, true);
        std::vector<uint32_t> colors;

        if (cstart && cend)
            colors = Tweening::RGB(Tweening::Gradient{cstart, cend, lines.size()});

        for (int i = 0; i < lines.size(); ++i) {
            if (cstart && cend)
                plane->set_fg_rgb(colors[i]);

            // Write strings directly onto the target plane
            plane->putstr(lines[i].c_str());
        }
    }

    int getHeight() const { return height; }

private:
    char hardblank = '$';
    int height = 0;
    int baseline = 0;
    int max_length = 0;
    int old_layout = 0;
    int comment_lines = 0;

    std::unordered_map<char32_t, std::vector<std::string>> glyphs;

    // Simple robust custom UTF-8 state decoder helper loop
    static std::vector<char32_t> decodeUtf8(std::string_view text) {
        std::vector<char32_t> codepoints;
        size_t i = 0;
        while (i < text.size()) {
            unsigned char c = text[i];
            char32_t cp = 0;
            size_t bytes = 0;

            if (c <= 0x7F) {
                cp = c;
                bytes = 1;
            } else if ((c & 0xE0) == 0xC0) {
                cp = c & 0x1F;
                bytes = 2;
            } else if ((c & 0xF0) == 0xE0) {
                cp = c & 0x0F;
                bytes = 3;
            } else if ((c & 0xF8) == 0xF0) {
                cp = c & 0x07;
                bytes = 4;
            } else {
                cp = c;
                bytes = 1;
            }

            if (i + bytes <= text.size()) {
                for (size_t b = 1; b < bytes; ++b) {
                    cp = (cp << 6) | (static_cast<unsigned char>(text[i + b]) & 0x3F);
                }
                i += bytes;
            } else {
                cp = c;
                i += 1;
            }
            codepoints.push_back(cp);
        }
        return codepoints;
    }

    std::vector<std::string> renderToStrings(const std::string_view text, bool append_nl=false) const
    {
        std::vector<std::string> ret;

        // Decode UTF-8 code units to UTF-32 code points for font matching
        std::vector<char32_t> codepoints = decodeUtf8(text);

        // Process the string slice line by line
        for (int r = 0; r < height; ++r) {
            std::string line;

            for (char32_t cp : codepoints) {
                auto it = glyphs.find(cp);

                if (it != glyphs.end() && r < it->second.size()) {
                    line += it->second[r];
                } else {
                    // Fallback to a space glyph if a character is missing
                    auto space_it = glyphs.find(U' ');

                    if (space_it != glyphs.end() && r < space_it->second.size()) {
                        line += space_it->second[r];
                    }
                }
            }

            if (append_nl) line += '\n';

            ret.emplace_back(line);
        }

        return ret;
    }
};
