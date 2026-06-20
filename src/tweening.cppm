module;

#include <cmath>
#include <cstdint>
#include <vector>

export module nctui.tweening;

export class Tweening
{
private:
    // Extract RGB values from a color
    static std::tuple<uint8_t, uint8_t, uint8_t> extractRGB(uint32_t color)
    {
        return std::tuple<uint8_t, uint8_t, uint8_t>(
            (color >> 16) & 0xFF,
            (color >> 8) & 0xFF,
            color & 0xFF
        );
    }

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

    static std::vector<uint32_t> Linear(Gradient gradient)
    {
        std::vector<uint32_t> result;

        if (gradient.steps == 0) {
            return result;
        }

        if (gradient.steps == 1) {
            result.push_back(gradient.start);
            return result;
        }

        // Extract the RGB components of start and end
        auto [r1, g1, b1] = extractRGB(gradient.start);
        auto [r2, g2, b2] = extractRGB(gradient.end);

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
        std::vector<uint32_t> gradientY = Linear(vertical);

        // Interpolate the horizontal gradient
        std::vector<uint32_t> gradientX = Linear(horizontal);

        // Create the matrix
        std::vector<std::vector<uint32_t>> matrix(
            vertical.steps,
            std::vector<uint32_t>(horizontal.steps)
        );

        for (uint32_t y = 0; y < vertical.steps; ++y) {
            for (uint32_t x = 0; x < horizontal.steps; ++x) {
                // Extract RGB values from the two gradients
                auto [r1, g1, b1] = extractRGB(gradientY[y]);
                auto [r2, g2, b2] = extractRGB(gradientX[x]);

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
        auto [r1, g1, b1] = extractRGB(gradient.start);
        auto [r2, g2, b2] = extractRGB(gradient.end);

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
