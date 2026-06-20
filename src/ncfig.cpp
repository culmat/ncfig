#include <ncpp/NotCurses.hh>
#include <ncpp/Plane.hh>

#include <iostream>
#include <string>
#include <string_view>

using namespace std::string_view_literals;

import nctui.cmdline;
import nctui.figlet;
import nctui.tweening;

auto help = "Usage: ncfig -f <font> [-v <gradient>] [-r <gradient>] [-h <gradient>] [text]\n"
            "\n"
            "  -f <font>      : Specify the font file to use\n"
            "  -v <gradient>  : Specify the vertical color gradient\n"
            "  -h <gradient>  : Specify the horizontal color gradient\n"
            "  -r <gradient>  : Specify the radial color gradient\n"
            "  text           : The text to render\n"
            "\n"
            "Gradients are specified as pairs of hexadecimal colors, e.g.\n"
            "0xff0000:0x00ff00 for a red-to-green gradient. They can optionally have a center\n"
            "color, e.g. 0xff0000:0x00ff00:0x0000ff for a red-to-green-to-blue gradient.\n"
            "\n"
            "Linear and vertical gradients can be combined (the two colors\n"
            "will be added together); radial gradients are exclusive.\n"sv;

int main(int argc, char** argv)
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

    // Get gradients from command line options
    auto[first, second] = opts.getGradients();

    // Get the text to display
    std::string text = opts.getText();

    if (text.empty()) {
        text = "HELLO NOTCURSES!";
    }

    if (filename.empty()) {
        std::cout << help;

        return EXIT_FAILURE;
    }

    notcurses_options options{
        .flags = NCOPTION_SUPPRESS_BANNERS | NCOPTION_CLI_MODE
    };

    auto nc = ncpp::NotCurses(options);
    ncpp::Plane* std_plane = nc.get_stdplane();

    FigletFont font;

    if (!font.loadFromFile(filename)) {
        std::cerr << "Failed to parse font file.\n";
        return EXIT_FAILURE;
    }

    // Render using the current cursor position
    std_plane->putstr("\n");
    font.render(std_plane, text, first, second);
    nc.render();

    return EXIT_SUCCESS;
}
