/**
 * @file tweening.cppm
 * @brief Color gradient generation module for NotCurses applications.
 *
 * This module provides functionality for generating linear, radial, and parallel
 * color gradients for use with NotCurses rendering. Colors are represented
 * as 32-bit values in 0xRRGGBB format.
 */

module;

#include <cmath>
#include <cstdint>
#include <vector>

export module nctui.tweening;

/**
 * @brief Provides color gradient generation for NotCurses terminal rendering.
 *
 * The Tweening class contains static methods for generating various types of
 * color gradients. All methods work with colors in 0xRRGGBB format (32-bit
 * unsigned integers where bits 16-23 are red, 8-15 are green, and 0-7 are blue).
 */
export class Tweening
{
private:
    /**
     * @brief Extracts RGB components from a 32-bit color value.
     *
     * @param color The color value in 0xRRGGBB format.
     * @return A tuple containing (red, green, blue) components as uint8_t values.
     */
    static std::tuple<uint8_t, uint8_t, uint8_t> extractRGB(uint32_t color)
    {
        return std::tuple<uint8_t, uint8_t, uint8_t>(
                   (color >> 16) & 0xFF,
                   (color >> 8) & 0xFF,
                   color & 0xFF
               );
    }

public:
    /**
     * @brief Defines the type of gradient to generate.
     */
    enum class GradientType {
        Unconfigured, ///< Default/unset gradient type.
        Linear,       ///< Linear gradient (1D color transition).
        Radial,       ///< Radial gradient (circular color transition from center).
    };

    /**
     * @brief Defines the orientation of a linear gradient.
     */
    enum class Orientation {
        Unconfigured, ///< Default/unset orientation.
        Horizontal,   ///< Gradient progresses left to right.
        Vertical,     ///< Gradient progresses top to bottom.
    };

    /**
     * @brief Configuration structure for defining a gradient.
     *
     * The center color field serves dual purposes:
     * - When center is 0: Two-color gradient (start to end)
     * - When center is non-zero: Three-color gradient (start to center to end)
     */
    struct Gradient {
        GradientType type;       ///< Type of gradient (Linear or Radial).
        Orientation orientation; ///< Direction of gradient (Horizontal or Vertical).

        std::uint32_t start;  ///< Starting color in 0xRRGGBB format.
        std::uint32_t center; ///< Center/middle color in 0xRRGGBB format (optional).
        std::uint32_t end;    ///< Ending color in 0xRRGGBB format.

        std::size_t steps;   ///< Number of interpolation steps (for linear gradients).
        std::size_t width;   ///< Width in pixels (for radial gradients).
        std::size_t height;  ///< Height in pixels (for radial gradients).
    };

    /**
     * @brief Generates a linear color gradient as a 1D vector of colors.
     *
     * Generates a sequence of colors by interpolating between start and end
     * colors. If a center color is provided (non-zero), creates a three-color
     * gradient (start to center to end) with the center at the midpoint.
     *
     * @param gradient The gradient configuration.
     * @return A vector of interpolated colors in 0xRRGGBB format.
     * @retval Empty vector if gradient.steps is 0.
     * @retval Vector with only start color if gradient.steps is 1.
     *
     * @note Uses linear interpolation for each RGB channel independently.
     */
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
        auto[r1, g1, b1] = extractRGB(gradient.start);
        auto[r2, g2, b2] = extractRGB(gradient.end);

        result.reserve(gradient.steps);

        if (!gradient.center) {
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
        } else {
            auto[rc, gc, bc] = extractRGB(gradient.center);

            for (uint32_t i = 0; i < gradient.steps; ++i) {
                float t = static_cast<float>(i) / (gradient.steps - 1);
                uint8_t r, g, b;

                if (t <= 0.5f) {
                    // start → center (t = 0 to 0.5)
                    float t_half = t * 2.0f;
                    r = static_cast<uint8_t>(r1 + (rc - r1) * t_half);
                    g = static_cast<uint8_t>(g1 + (gc - g1) * t_half);
                    b = static_cast<uint8_t>(b1 + (bc - b1) * t_half);
                } else {
                    // center → end (t = 0.5 to 1)
                    float t_half = (t - 0.5f) * 2.0f;
                    r = static_cast<uint8_t>(rc + (r2 - rc) * t_half);
                    g = static_cast<uint8_t>(gc + (g2 - gc) * t_half);
                    b = static_cast<uint8_t>(bc + (b2 - bc) * t_half);
                }

                result.push_back((r << 16) | (g << 8) | b);
            }
        }

        return result;
    }

    /**
     * @brief Generates a 2D color matrix by combining vertical and horizontal gradients.
     *
     * Creates a 2D matrix where each pixel is computed by adding the RGB channels
     * of the corresponding vertical and horizontal gradient colors (modulo 256).
     *
     * @param vertical The gradient configuration for the vertical direction.
     * @param horizontal The gradient configuration for the horizontal direction.
     * @return A 2D vector (matrix) of colors with dimensions vertical.steps x horizontal.steps.
     *
     * @note Color combination formula: result = (vertical_color + horizontal_color) % 256
     *       for each RGB channel independently.
     */
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
                auto[r1, g1, b1] = extractRGB(gradientY[y]);
                auto[r2, g2, b2] = extractRGB(gradientX[x]);

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

    /**
     * @brief Generates a radial (circular) color gradient as a 2D matrix.
     *
     * Creates a circular gradient radiating outward from the center of the
     * matrix. The start color is at the center, and the end color is at the
     * edges.  If a center color is provided (non-zero), creates a three-color
     * gradient with the center color at 50% radius.
     *
     * @param gradient The gradient configuration with width and height defining output dimensions.
     * @return A 2D vector (matrix) of colors with dimensions gradient.height x gradient.width.
     *
     * @note The gradient uses Euclidean distance from the center, normalized to [0, 1].
     *       For three-color gradients, the transition is: start (center) to center (50% radius)
     *       to end (edges).
     */
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

        // Extract RGB values of start, center, and end colors
        auto[r1, g1, b1] = extractRGB(gradient.start);
        auto[r2, g2, b2] = extractRGB(gradient.end);
        auto[rc, gc, bc] = extractRGB(gradient.center);

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
                uint8_t r, g, b;

                if (!gradient.center) {
                    // Two-color gradient: start to end
                    r = static_cast<uint8_t>(r1 + (r2 - r1) * t);
                    g = static_cast<uint8_t>(g1 + (g2 - g1) * t);
                    b = static_cast<uint8_t>(b1 + (b2 - b1) * t);
                } else {
                    // Three-color gradient: start to center to end
                    if (t <= 0.5f) {
                        // start -> center (t = 0 to 0.5)
                        float t_half = t * 2.0f;
                        r = static_cast<uint8_t>(r1 + (rc - r1) * t_half);
                        g = static_cast<uint8_t>(g1 + (gc - g1) * t_half);
                        b = static_cast<uint8_t>(b1 + (bc - b1) * t_half);
                    } else {
                        // center -> end (t = 0.5 to 1)
                        float t_half = (t - 0.5f) * 2.0f;
                        r = static_cast<uint8_t>(rc + (r2 - rc) * t_half);
                        g = static_cast<uint8_t>(gc + (g2 - gc) * t_half);
                        b = static_cast<uint8_t>(bc + (b2 - bc) * t_half);
                    }
                }

                matrix[i][j] = (r << 16) | (g << 8) | b;
            }
        }

        return matrix;
    }
};
