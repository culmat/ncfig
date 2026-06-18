module;

#include <ncpp/Plane.hh>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

export module figlet;

export class Tweening
{
public:
    enum class GradientType {
        Unconfigured,
        Linear,
        Radial,
    };

    enum class Orientation {
        Unconfigured,
        Horizontal,
        Vertical,
    };

    struct Gradient {
        GradientType type;
        Orientation orientation;

        std::uint32_t start;
        std::uint32_t end;

        // linear interpolation steps
        std::size_t steps;

        // radial interpolation steps
        std::size_t width;
        std::size_t height;
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

    static std::vector<std::vector<uint32_t>> Parallel(
        Gradient vertical,
        Gradient horizontal)
    {
        // Interpolate the vertical gradient
        std::vector<uint32_t> gradientY = RGB(vertical);

        // Interpolate the horizontal gradient
        std::vector<uint32_t> gradientX = RGB(horizontal);

        // Create the matrix
        std::vector<std::vector<uint32_t>> matrix(
            vertical.steps,
            std::vector<uint32_t>(horizontal.steps)
        );

        for (uint32_t y = 0; y < vertical.steps; ++y) {
            for (uint32_t x = 0; x < horizontal.steps; ++x) {
                // Extract RGB values from the two gradients
                uint8_t r1 = (gradientY[y] >> 16) & 0xFF;
                uint8_t g1 = (gradientY[y] >> 8) & 0xFF;
                uint8_t b1 = gradientY[y] & 0xFF;

                uint8_t r2 = (gradientX[x] >> 16) & 0xFF;
                uint8_t g2 = (gradientX[x] >> 8) & 0xFF;
                uint8_t b2 = gradientX[x] & 0xFF;

                // Combine colors (here: addition modulo 256)
                uint8_t r = (r1 + r2) % 256;
                uint8_t g = (g1 + g2) % 256;
                uint8_t b = (b1 + b2) % 256;

                // Store the result in the matrix
                matrix[y][x] = (r << 16) | (g << 8) | b;
            }
        }

        return matrix;
    }

    static std::vector<std::vector<uint32_t>> radial(Gradient gradient)
    {
        // Calculate center
        float centerX = (gradient.width - 1) / 2.0f;
        float centerY = (gradient.height - 1) / 2.0f;

        // Calculate max distance
        float maxDist = std::sqrt(
            std::pow(std::max(centerX, gradient.width - 1 - centerX), 2) +
            std::pow(std::max(centerY, gradient.height - 1 - centerY), 2)
        );

        // Extract RGB values of start and end colors
        uint8_t r1 = (gradient.start >> 16) & 0xFF;
        uint8_t g1 = (gradient.start >> 8) & 0xFF;
        uint8_t b1 = gradient.start & 0xFF;

        uint8_t r2 = (gradient.end >> 16) & 0xFF;
        uint8_t g2 = (gradient.end >> 8) & 0xFF;
        uint8_t b2 = gradient.end & 0xFF;

        // Create matrix
        std::vector<std::vector<uint32_t>> matrix(
            gradient.height,
            std::vector<uint32_t>(gradient.width)
        );

        for (uint32_t i = 0; i < gradient.height; ++i) {
            for (uint32_t j = 0; j < gradient.width; ++j) {
                // Calculate distance to center
                float dx = j - centerX;
                float dy = i - centerY;
                float dist = std::sqrt(dx * dx + dy * dy);

                // Normalize distance to [0, 1]
                float t = std::min(dist / maxDist, 1.0f);

                // Interpolate color
                uint8_t r = static_cast<uint8_t>(r1 + (r2 - r1) * t);
                uint8_t g = static_cast<uint8_t>(g1 + (g2 - g1) * t);
                uint8_t b = static_cast<uint8_t>(b1 + (b2 - b1) * t);

                matrix[i][j] = (r << 16) | (g << 8) | b;
            }
        }

        return matrix;
    }
};

export class FigletFont
{
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
     * @param first First gradient for the text.
     * @param second Second gradient for the text.
     */
    void render(
        ncpp::Plane* plane,
        int y,
        int x,
        std::string_view text,
        Tweening::Gradient first = {},
        Tweening::Gradient second = {}
    ) const
    {
        if (height <= 0) return;

        auto lines = renderToStrings(text);
        auto [
            one_d,
            two_d,
            max_width
        ] = prepareGradients(lines, first, second);

        for (int i = 0; i < lines.size(); ++i) {
            if (!two_d.empty()) {
                // 2d tweening
                // print by character
                for (int j = 0; j < lines[i].size(); ++j) {
                    plane->set_fg_rgb(two_d[i][j]);
                    plane->putc(y + i, x + j, lines[i][j]);
                }
            } else if (!one_d.empty() && first.orientation == Tweening::Orientation::Vertical) {
                plane->set_fg_rgb(one_d[i]);
                // Write strings directly onto the target plane coordinates
                plane->putstr(y + i, x, lines[i].c_str());
            } else if (!one_d.empty() && first.orientation == Tweening::Orientation::Horizontal) {
                // print by character
                for (int j = 0; j < lines[i].size(); ++j) {
                    plane->set_fg_rgb(one_d[j]);
                    plane->putc(y + i, x + j, lines[i][j]);
                }
            } else {
                plane->putstr(y + i, x, lines[i].c_str());
            }
        }
    }

    /**
     * @brief Renders text onto an ncpp::Plane using a horizontal full-width alignment.
     * @param plane The target Notcurses C++ plane.
     * @param text String view containing UTF-8 input text.
     * @param first First gradient for the text.
     * @param second Second gradient for the text.
     */
    void render(
        ncpp::Plane* plane,
        std::string_view text,
        Tweening::Gradient first = {},
        Tweening::Gradient second = {}
    ) const
    {
        if (height <= 0) return;

        auto lines = renderToStrings(text, true);
        auto [
            one_d,
            two_d,
            max_width
        ] = prepareGradients(lines, first, second);

        for (int i = 0; i < lines.size(); ++i) {
            if (!two_d.empty()) {
                // 2d tweening
                // print by character
                for (int j = 0; j < lines[i].size(); ++j) {
                    plane->set_fg_rgb(two_d[i][j]);
                    plane->putc(lines[i][j]);
                }
            } else if (!one_d.empty() && first.orientation == Tweening::Orientation::Vertical) {
                plane->set_fg_rgb(one_d[i]);
                plane->putstr(lines[i].c_str());
            } else if (!one_d.empty() && first.orientation == Tweening::Orientation::Horizontal) {
                // print by character
                for (int j = 0; j < lines[i].size(); ++j) {
                    plane->set_fg_rgb(one_d[j]);
                    plane->putc(lines[i][j]);
                }
            } else {
                plane->putstr(lines[i].c_str());
            }
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

    /**
     * @brief Prepares the gradients and colors for rendering.
     * @param lines The lines of text.
     * @param first First gradient.
     * @param second Second gradient.
     * @return A tuple with (one_d, two_d, max_width).
     */
    std::tuple<
        std::vector<uint32_t>,
        std::vector<std::vector<uint32_t>>,
        std::size_t
    > prepareGradients(
        const std::vector<std::string>& lines,
        Tweening::Gradient first,
        Tweening::Gradient second
    ) const {
        std::vector<uint32_t> one_d;
        std::vector<std::vector<uint32_t>> two_d;
        std::size_t max_width = 0;

        for (const auto& line : lines) {
            max_width = std::max(max_width, line.size());
        }

        if (first.type == Tweening::GradientType::Linear
            && second.type == Tweening::GradientType::Linear)
        {
            // this combination is only supported for parallel linear
            // gradients, so we can ignore orientation
            first.steps = lines.size();
            second.steps = max_width;
            two_d = Tweening::Parallel(first, second);
        }
        else if (first.type == Tweening::GradientType::Radial)
        {
            // Radial gradient
            first.width = max_width;
            first.height = lines.size();
            two_d = Tweening::radial(first);
        }
        else if (first.type == Tweening::GradientType::Linear
            && first.orientation == Tweening::Orientation::Vertical)
        {
            // One-dimensional gradient (vertical)
            first.steps = lines.size();
            one_d = Tweening::RGB(first);
        }
        else if (first.type == Tweening::GradientType::Linear
            && first.orientation == Tweening::Orientation::Horizontal)
        {
            // One-dimensional gradient (horizontal)
            first.steps = max_width;
            one_d = Tweening::RGB(first);
        }

        return {one_d, two_d, max_width};
    }
};
