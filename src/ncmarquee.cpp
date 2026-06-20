#include <ncpp/NotCurses.hh>
#include <ncpp/Plane.hh>
#include <ncpp/NCKey.hh>

#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace std::string_view_literals;
using namespace std::chrono_literals;

import nctui.cmdline;
import nctui.figlet;
import nctui.tweening;

auto help = "Usage: ncmarquee -f <font> [-v <gradient>] [-h <gradient>] [-r <gradient>] [-s <ms>] [-t] [text]\n"
            "\n"
            "  -f <font>     : Specify the font file to use\n"
            "  -v <gradient> : Specify the vertical color gradient\n"
            "  -h <gradient> : Specify the horizontal color gradient\n"
            "  -r <gradient> : Specify the radial color gradient\n"
            "  -s <ms>       : Scroll speed in milliseconds (default: 50)\n"
            "  -t            : Apply gradient to text dimensions\n"
            "                  (default: apply to the visible area of the text)\n"
            "  text          : The text to render (default: HELLO NOTCURSES!)\n"
            "\n"
            "Gradients are specified as pairs of hexadecimal colors, e.g.\n"
            "0xff0000:0x00ff00 for a red-to-green gradient. They can optionally have a center\n"
            "color, e.g. 0xff0000:0x00ff00:0x0000ff for a red-to-green-to-blue gradient.\n"
            "\n"
            "Linear and vertical gradients can be combined (the two colors\n"
            "will be added together); radial gradients are exclusive.\n"
            "\n"
            "Creates a full-screen marquee animation. Press any key to exit.\n"sv;

int main(int argc, char **argv)
{
    CmdLine opts(argc, argv);

    if (opts.cmdOptionExists("-?") || opts.cmdOptionExists("--help")) {
        std::cout << help;

        return EXIT_SUCCESS;
    }

    std::string filename = opts.getCmdOption("-f");

    // Validate gradient options
    if (!opts.validateGradients()) {
        std::cerr << "Error: radial and vertical gradients cannot be combined.\n";
        std::cout << help;

        return EXIT_FAILURE;
    }

    // Parse speed option (default: 50ms)
    int delay_ms = 50;
    std::string speed_str = opts.getCmdOption("-s");
    if (!speed_str.empty()) {
        try {
            delay_ms = std::stoi(speed_str);
        } catch (...) {
            std::cerr << "Error: Invalid speed value. Must be an integer.\n";
            std::cout << help;

            return EXIT_FAILURE;
        }
    }

    // Check if gradient should be applied to text dimensions
    bool gradient_to_text = opts.cmdOptionExists("-t");

    // Get gradients from command line options
    auto [first, second] = opts.getGradients();

    // Get the text to display
    std::string text = opts.getText();

    if (text.empty()) {
        text = "HELLO NOTCURSES!";
    }

    if (filename.empty()) {
        std::cout << help;

        return EXIT_FAILURE;
    }

    // Initialize NotCurses in full-screen mode (not CLI mode)
    notcurses_options nc_opts{};
    auto nc = ncpp::NotCurses(nc_opts);
    ncpp::Plane* std_plane = nc.get_stdplane();

    // Get the standard plane dimensions
    int screen_width = std_plane->get_dim_x();
    int screen_height = std_plane->get_dim_y();

    FigletFont font;

    if (!font.loadFromFile(filename)) {
        std::cerr << "Failed to parse font file.\n";

        return EXIT_FAILURE;
    }

    // Get the dimensions of the rendered text
    auto [text_width, text_height] = font.getTextDimensions(text);

    if (text_width == 0 || text_height == 0) {
        std::cerr << "Failed to render text or text has no dimensions.\n";

        return EXIT_FAILURE;
    }

    // Calculate vertical position to center the text
    int y_pos = (screen_height - text_height) / 2;

    // Position for marquee animation
    // Start off-screen to the right
    int x_pos = screen_width;

    // Animation loop
    bool running = true;

    while (running) {
        // Clear the standard plane
        std_plane->erase();

        // Use the new renderToViewport method
        font.renderToViewport(
            std_plane,
            y_pos,
            x_pos,
            text,
            first,
            second,
            screen_width,
            x_pos,
            gradient_to_text
        );

        // Render the frame
        nc.render();

        // Move the text to the left
        x_pos--;

        // If text has completely left the visible area, reset to right side
        // The text is completely off-screen when x_pos + text_width < 0
        if (x_pos + text_width < 0) {
            x_pos = screen_width;  // Reset to just off-screen on the right
        }

        // Check for key press (non-blocking)
        ncinput input{};
        uint32_t key = nc.get(false, &input);

        // Check if a key was pressed (any key except timeout/none)
        // In non-blocking mode, get returns 0 on timeout
        if (key != 0) {
            running = false;
        }

        // Add delay between frames
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }

    return EXIT_SUCCESS;
}
