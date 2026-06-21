module;

#include <ncpp/Plane.hh>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

export module nctui.figlet;

import nctui.tweening;

export class FigletFont
{
public:
    FigletFont() = default;

    /**
     * @brief Loads and parses a standard FIGlet font (.flf) file.
     *
     * @param filename Path to the .flf file.
     * @return true if successfully loaded and parsed, false otherwise.
     */
    bool loadFromFile(const std::string& filename)
    {
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
            if (code_line.empty()) {
                continue;
            }

            std::istringstream code_iss(code_line);
            std::string code_str;
            code_iss >> code_str;

            if (code_str.empty()) {
                continue;
            }

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

            if (!success) {
                break;
            }

            glyphs[code] = std::move(glyph_rows);
        }

        return true;
    }

    /**
     * @brief Renders text onto an ncpp::Plane using a horizontal full-width alignment.
     *
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
        if (height <= 0) {
            return;
        }

        auto lines = renderToStrings(text);
        auto[
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
     *
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
        if (height <= 0) {
            return;
        }

        auto lines = renderToStrings(text, true);
        auto[
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

    /**
     * @brief Renders a portion of text onto a plane with gradients applied to viewport or text dimensions.
     *
     * This is useful for marquee-style rendering where text scrolls across a fixed viewport.
     *
     * @param plane The target Notcurses C++ plane.
     * @param y Top-left Y row coordinate.
     * @param x Top-left X column coordinate (starting position on plane).
     * @param text String view containing UTF-8 input text.
     * @param first First gradient for the text.
     * @param second Second gradient for the text.
     * @param viewport_width Width of the viewport.
     * @param offset Horizontal offset into the text (for scrolling).
     * @param use_text_dimensions If true, use text dimensions for gradients; otherwise use viewport width x text height.
     * @param wave_amplitude Amplitude of the wave effect (default 0).
     * @param wave_phase Phase of the wave effect (default 0.0f).
     */
    void renderToViewport(
        ncpp::Plane* plane,
        int y,
        int x,
        std::string_view text,
        Tweening::Gradient first,
        Tweening::Gradient second,
        int viewport_width,
        int offset = 0,
        bool use_text_dimensions = false,
        int wave_amplitude = 0,
        float wave_phase = 0.0f
    ) const
    {
        if (height <= 0) {
            return;
        }

        // Render text to strings
        auto lines = renderToStrings(text);

        if (lines.empty()) {
            return;
        }

        int text_height = lines.size();
        int text_width = 0;

        for (const auto& line : lines) {
            text_width = std::max(text_width, static_cast<int>(line.size()));
        }

        // Prepare gradients
        // If use_text_dimensions, use text dimensions; otherwise use viewport width x text height
        int gradient_width = use_text_dimensions ? text_width : viewport_width;
        int gradient_height = text_height + 2 * wave_amplitude;

        if (first.type == Tweening::GradientType::Linear
                && second.type == Tweening::GradientType::Linear) {
            // Parallel gradients
            first.steps = gradient_height;
            second.steps = gradient_width;
        } else if (first.type == Tweening::GradientType::Radial) {
            first.width = gradient_width;
            first.height = gradient_height;
        } else if (first.type == Tweening::GradientType::Linear
                   && first.orientation == Tweening::Orientation::Vertical) {
            // Vertical: steps = gradient height
            first.steps = gradient_height;
        } else if (first.type == Tweening::GradientType::Linear
                   && first.orientation == Tweening::Orientation::Horizontal) {
            // Horizontal: steps = gradient width
            first.steps = gradient_width;
        }

        auto[one_d, two_d, max_width] = prepareGradients(lines, first, second);

        int plane_height = plane->get_dim_y();

        // Map every text column → its character's center x (in text coordinates).
        // Snapping per-column y to the character center keeps each glyph internally
        // coherent during wave rendering (all columns of one glyph share one y offset).
        std::vector<float> col_center;

        if (wave_amplitude != 0) {
            col_center.resize(text_width, 0.0f);
            auto char_ranges = buildCharRanges(text);

            for (const auto& [s, e] : char_ranges) {
                float center = (s + e - 1) * 0.5f;

                for (int c = s; c < e && c < text_width; ++c) {
                    col_center[c] = center;
                }
            }
        }

        int line_offset = 0;
        int visible_width = 0;

        if (offset < viewport_width && offset + text_width > 0) {
            line_offset = std::max(0, -offset);
            visible_width = std::min(text_width - line_offset, viewport_width - std::max(0, offset));
        }

        if (visible_width <= 0) {
            return;
        }

        int screen_x = std::max(0, offset);

        // Outer loop over columns so y_off (and the sin() it needs) is computed
        // once per column rather than once per (row, column) pair.
        for (int j = 0; j < visible_width; ++j) {
            int text_x = line_offset + j;
            int screen_j = screen_x + j;

            // Compute wave y-offset for this column (0 when wave_amplitude == 0).
            // -t off: phase from screen position → wave is fixed in screen space,
            //         circular instances share the same continuous wave.
            // -t on:  phase from text position → wave travels with the text.
            int y_off = 0;

            if (wave_amplitude != 0 && text_x < (int)col_center.size()) {
                constexpr float wave_spatial_freq = 2.0f * 3.14159265f / 150.0f;
                float char_x = use_text_dimensions
                               ? col_center[text_x]
                               : static_cast<float>(offset) + col_center[text_x];
                float phase = wave_phase + char_x * wave_spatial_freq;
                y_off = static_cast<int>(std::round(wave_amplitude * std::sin(phase)));
            }

            int col_y = std::clamp(y + y_off, 0, std::max(0, plane_height - text_height));
            int color_x = use_text_dimensions ? text_x : screen_j;

            for (int i = 0; i < text_height; ++i) {
                if (text_x >= (int)lines[i].size()) {
                    break;
                }

                if (col_y + i >= plane_height) {
                    continue;
                }

                // Get color based on position.
                // color_y shifts with the wave so colors follow the character's screen position.
                uint32_t color = 0xFFFFFF;
                int color_y = wave_amplitude + y_off + i;

                if (color_y < (int)two_d.size() && color_x < (int)two_d[color_y].size()) {
                    color = two_d[color_y][color_x];
                } else if (first.orientation == Tweening::Orientation::Vertical
                           && color_y < (int)one_d.size()) {
                    color = one_d[color_y];
                } else if ((first.orientation == Tweening::Orientation::Horizontal
                            || (first.type == Tweening::GradientType::Linear && second.type == Tweening::GradientType::Linear))
                           && color_x < (int)one_d.size()) {
                    color = one_d[color_x];
                }

                plane->set_fg_rgb(color);
                plane->putc(col_y + i, screen_j, lines[i][text_x]);
            }
        }
    }

    int getHeight() const
    {
        return height;
    }

    /**
     * @brief Returns the dimensions of rendered text.
     * @param text String view containing UTF-8 input text.
     * @return A pair of (width, height) in columns and rows.
     */
    std::pair<int, int> getTextDimensions(std::string_view text) const
    {
        if (height <= 0) {
            return {0, 0};
        }

        auto lines = renderToStrings(text);

        if (lines.empty()) {
            return {0, 0};
        }

        int max_width = 0;

        for (const auto& line : lines) {
            max_width = std::max(max_width, static_cast<int>(line.size()));
        }

        return {max_width, static_cast<int>(lines.size())};
    }

private:
    char hardblank = '$';
    int height = 0;
    int baseline = 0;
    int max_length = 0;
    int old_layout = 0;
    int comment_lines = 0;

    std::unordered_map<char32_t, std::vector<std::string>> glyphs;

    // Simple robust custom UTF-8 state decoder helper loop
    static std::vector<char32_t> decodeUtf8(std::string_view text)
    {
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

    // Returns {start_col, end_col} for every character in the rendered text.
    std::vector<std::pair<int, int>> buildCharRanges(std::string_view sv) const
    {
        std::vector<std::pair<int, int>> ranges;
        auto codepoints = decodeUtf8(sv);
        int x = 0;

        for (char32_t cp : codepoints) {
            int w = 0;
            auto it = glyphs.find(cp);

            if (it != glyphs.end() && !it->second.empty()) {
                w = static_cast<int>(it->second[0].size());
            } else {
                auto space_it = glyphs.find(U' ');

                if (space_it != glyphs.end() && !space_it->second.empty()) {
                    w = static_cast<int>(space_it->second[0].size());
                }
            }

            ranges.push_back({x, x + w});
            x += w;
        }

        return ranges;
    }

    std::vector<std::string> renderToStrings(const std::string_view text, bool append_nl = false) const
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

            if (append_nl) {
                line += '\n';
            }

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
    std::tuple <
    std::vector<uint32_t>,
        std::vector<std::vector<uint32_t>>,
        std::size_t
        > prepareGradients(
            const std::vector<std::string>& lines,
            Tweening::Gradient first,
            Tweening::Gradient second
        ) const
    {
        std::vector<uint32_t> one_d;
        std::vector<std::vector<uint32_t>> two_d;
        std::size_t max_width = 0;

        for (const auto& line : lines) {
            max_width = std::max(max_width, line.size());
        }

        if (first.type == Tweening::GradientType::Linear
                && second.type == Tweening::GradientType::Linear) {
            // this combination is only supported for parallel linear
            // gradients, so we can ignore orientation
            if (first.steps == 0) {
                first.steps = lines.size();
            }
            if (second.steps == 0) {
                second.steps = max_width;
            }
            two_d = Tweening::Parallel(first, second);
        } else if (first.type == Tweening::GradientType::Radial) {
            // Radial gradient
            if (first.width == 0) {
                first.width = max_width;
            }
            if (first.height == 0) {
                first.height = lines.size();
            }
            two_d = Tweening::radial(first);
        } else if (first.type == Tweening::GradientType::Linear
                   && first.orientation == Tweening::Orientation::Vertical) {
            // One-dimensional gradient (vertical)
            if (first.steps == 0) {
                first.steps = lines.size();
            }

            one_d = Tweening::Linear(first);
        } else if (first.type == Tweening::GradientType::Linear
                   && first.orientation == Tweening::Orientation::Horizontal) {
            // One-dimensional gradient (horizontal)
            if (first.steps == 0) {
                first.steps = max_width;
            }

            one_d = Tweening::Linear(first);
        }

        return {one_d, two_d, max_width};
    }
};
