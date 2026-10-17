module;

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

export module nctui.canvas;

/**
 * @brief Represents a single character cell with 24-bit RGB foreground color.
 */
export struct Cell
{
    char ch = ' ';
    uint32_t fg = 0xFFFFFF; // 0x00RRGGBB format

    [[nodiscard]] constexpr uint8_t r() const noexcept
    {
        return static_cast<uint8_t>((fg >> 16) & 0xFF);
    }

    [[nodiscard]] constexpr uint8_t g() const noexcept
    {
        return static_cast<uint8_t>((fg >> 8) & 0xFF);
    }

    [[nodiscard]] constexpr uint8_t b() const noexcept
    {
        return static_cast<uint8_t>(fg & 0xFF);
    }
};

/**
 * @brief A 2D grid of character cells with RGB colors.
 *
 * TextCanvas acts as an in-memory frame buffer for rendering FIGlet fonts,
 * color gradients, and text layouts independently of any specific display
 * or TUI library.
 */
export class TextCanvas
{
public:
    TextCanvas() = default;

    TextCanvas(int width, int height, Cell fill = {' ', 0xFFFFFF})
        : width_(std::max(0, width)),
          height_(std::max(0, height)),
          buffer_(width_ * height_, fill)
    {
    }

    [[nodiscard]] int width() const noexcept
    {
        return width_;
    }

    [[nodiscard]] int height() const noexcept
    {
        return height_;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return buffer_.empty();
    }

    void resize(int width, int height, Cell fill = {' ', 0xFFFFFF})
    {
        width_ = std::max(0, width);
        height_ = std::max(0, height);
        buffer_.assign(width_ * height_, fill);
    }

    void clear(Cell fill = {' ', 0xFFFFFF})
    {
        std::fill(buffer_.begin(), buffer_.end(), fill);
    }

    [[nodiscard]] bool inBounds(int y, int x) const noexcept
    {
        return x >= 0 && x < width_ && y >= 0 && y < height_;
    }

    void set(int y, int x, char ch, uint32_t fg) noexcept
    {
        if (inBounds(y, x)) {
            buffer_[y * width_ + x] = Cell{ch, fg};
        }
    }

    void set(int y, int x, const Cell& cell) noexcept
    {
        if (inBounds(y, x)) {
            buffer_[y * width_ + x] = cell;
        }
    }

    [[nodiscard]] const Cell& at(int y, int x) const noexcept
    {
        return buffer_[y * width_ + x];
    }

    [[nodiscard]] Cell& operator()(int y, int x) noexcept
    {
        return buffer_[y * width_ + x];
    }

    [[nodiscard]] const Cell& operator()(int y, int x) const noexcept
    {
        return buffer_[y * width_ + x];
    }

    [[nodiscard]] const Cell* row(int y) const noexcept
    {
        if (y >= 0 && y < height_) {
            return &buffer_[y * width_];
        }
        return nullptr;
    }

    [[nodiscard]] const std::vector<Cell>& cells() const noexcept
    {
        return buffer_;
    }

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<Cell> buffer_;
};
