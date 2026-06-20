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
     * This is useful for marquee-style rendering where text scrolls across a fixed viewport.
     * @param plane The target Notcurses C++ plane.
     * @param y Top-left Y row coordinate.
     * @param x Top-left X column coordinate (starting position on plane).
     * @param text String view containing UTF-8 input text.
     * @param first First gradient for the text.
     * @param second Second gradient for the text.
     * @param viewport_width Width of the viewport.
     * @param offset Horizontal offset into the text (for scrolling).
     * @param use_text_dimensions If true, use text dimensions for gradients; otherwise use viewport width x text height.
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
        int gradient_height = text_height;

        if (first.type == Tweening::GradientType::Linear
                && second.type == Tweening::GradientType::Linear) {
            // Parallel gradients
            if (first.steps == 0) first.steps = gradient_height;
            if (second.steps == 0) second.steps = gradient_width;
        } else if (first.type == Tweening::GradientType::Radial) {
            // Radial: width = gradient_width, height = gradient_height
            if (first.width == 0) first.width = gradient_width;
            if (first.height == 0) first.height = gradient_height;
        } else if (first.type == Tweening::GradientType::Linear
                   && first.orientation == Tweening::Orientation::Horizontal) {
            // Horizontal: steps = gradient width
            if (first.steps == 0) first.steps = gradient_width;
        }
        // Vertical gradient: steps = gradient height (already handled by prepareGradients)

        auto [one_d, two_d, max_width] = prepareGradients(lines, first, second);

        int plane_height = plane->get_dim_y();

        if (wave_amplitude != 0 && offset < viewport_width && offset + text_width > 0) {
            // Wave mode: render column by column, but snap each column's y offset to the
            // center x of its parent character so the whole character moves as a unit.
            // Without this, individual columns of the same glyph end up at different heights,
            // tearing the character apart. The inter-character staircase is unavoidable (no
            // sub-row precision in a terminal), but at least each glyph is internally coherent.
            constexpr float wave_spatial_freq = 2.0f * 3.14159265f / 150.0f;

            // Map every text column → its character's center x (in text coordinates).
            std::vector<float> col_center(text_width, 0.0f);
            {
                auto char_ranges = buildCharRanges(text);
                for (const auto& [s, e] : char_ranges) {
                    float center = (s + e - 1) * 0.5f;
                    for (int c = s; c < e && c < text_width; ++c) {
                        col_center[c] = center;
                    }
                }
            }

            int x_start = std::max(0, offset);
            int x_end   = std::min(viewport_width, offset + text_width);

            for (int screen_col = x_start; screen_col < x_end; ++screen_col) {
                int text_col = screen_col - offset;
                if (text_col >= text_width) {
                    continue;
                }

                // -t off: phase from screen position → wave is fixed in screen space,
                //         circular instances share the same continuous wave.
                // -t on:  phase from text position → wave travels with the text.
                float char_x = use_text_dimensions
                    ? col_center[text_col]
                    : static_cast<float>(offset) + col_center[text_col];
                float phase = wave_phase + char_x * wave_spatial_freq;
                int y_off = static_cast<int>(std::round(wave_amplitude * std::sin(phase)));
                int col_y = std::clamp(y + y_off, 0, std::max(0, plane_height - text_height));

                for (int i = 0; i < text_height; ++i) {
                    if (col_y + i >= plane_height) {
                        continue;
                    }
                    if (text_col >= (int)lines[i].size()) {
                        continue;
                    }

                    uint32_t color = 0xFFFFFF;
                    int color_x = use_text_dimensions ? text_col : screen_col;

                    if (!two_d.empty() && i < (int)two_d.size() && color_x < (int)two_d[i].size()) {
                        color = two_d[i][color_x];
                    } else if (!one_d.empty() && first.orientation == Tweening::Orientation::Vertical
                               && i < (int)one_d.size()) {
                        color = one_d[i];
                    } else if (!one_d.empty() && color_x < (int)one_d.size()) {
                        color = one_d[color_x];
                    }

                    plane->set_fg_rgb(color);
                    plane->putc(col_y + i, screen_col, lines[i][text_col]);
                }
            }
            return;
        }

        // Normal mode: render row by row.
        int text_end_pos = offset + text_width;
        int visible_width = 0;
        int line_offset = 0;

        if (offset < viewport_width && text_end_pos > 0) {
            line_offset = std::max(0, -offset);
            visible_width = std::min(text_width - line_offset, viewport_width - std::max(0, offset));
        }

        // Render each line
        for (int i = 0; i < text_height; ++i) {
            if (y + i < 0 || y + i >= plane_height) {
                continue;
            }

            if (visible_width > 0) {
                int screen_x = std::max(0, offset);

                for (int j = 0; j < visible_width; ++j) {
                    int text_x = line_offset + j;
                    int screen_j = screen_x + j;

                    if (text_x >= (int)lines[i].size()) {
                        break;
                    }

                    // Get color based on position
                    uint32_t color = 0xFFFFFF; // Default white
                    int color_x = use_text_dimensions ? text_x : screen_j;

                    if (!two_d.empty() && i < (int)two_d.size() && color_x < (int)two_d[i].size()) {
                        color = two_d[i][color_x];
                    } else if (!one_d.empty() && first.orientation == Tweening::Orientation::Vertical
                               && i < (int)one_d.size()) {
                        color = one_d[i];
                    } else if (!one_d.empty() && (first.orientation == Tweening::Orientation::Horizontal
                                || (first.type == Tweening::GradientType::Linear && second.type == Tweening::GradientType::Linear))
                               && color_x < (int)one_d.size()) {
                        color = one_d[color_x];
                    }

                    plane->set_fg_rgb(color);
                    plane->putc(y + i, screen_x + j, lines[i][text_x]);
                }
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
            first.steps = lines.size();
            second.steps = max_width;
            two_d = Tweening::Parallel(first, second);
        } else if (first.type == Tweening::GradientType::Radial) {
            // Radial gradient
            first.width = max_width;
            first.height = lines.size();
            two_d = Tweening::radial(first);
        } else if (first.type == Tweening::GradientType::Linear
                   && first.orientation == Tweening::Orientation::Vertical) {
            // One-dimensional gradient (vertical)
            if (first.steps == 0) first.steps = lines.size();
            one_d = Tweening::Linear(first);
        } else if (first.type == Tweening::GradientType::Linear
                   && first.orientation == Tweening::Orientation::Horizontal) {
            // One-dimensional gradient (horizontal)
            if (first.steps == 0) first.steps = max_width;
            one_d = Tweening::Linear(first);
        }

        return {one_d, two_d, max_width};
    }
};
