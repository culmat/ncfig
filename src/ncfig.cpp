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
            "Gradients are specified as pairs of hexadecimal colors, e.g. \n"
            "0xff0000:0x00ff00 for a red-to-green gradient.\n"
            "\n"
            "Linear and vertical gradients can be combined (the two colors\n"
            "will be added together); radial gradients are exclusive.\n"sv;

int main(int argc, char **argv)
{
    CmdLine opts(argc, argv);

    if (opts.cmdOptionExists("-?") || opts.cmdOptionExists("--help")) {
        std::cout << help;

        return EXIT_SUCCESS;
    }

    std::string filename = opts.getCmdOption("-f");
    std::string vgradient = opts.getCmdOption("-v");
    std::string hgradient = opts.getCmdOption("-h");
    std::string rgradient = opts.getCmdOption("-r");

    // Sanity check
    if (!rgradient.empty() && (!vgradient.empty() || !hgradient.empty())) {
        std::cerr << "Error: radial and vertical gradients cannot be combined.\n";
        std::cout << help;

        return EXIT_FAILURE;
    }

    // Get the text to display
    std::string text = opts.getText();

    if (text.empty()) {
        text = "HELLO NOTCURSES!";
    }

    if (filename.empty()) {
        std::cout << help;

        return EXIT_FAILURE;
    }

    Tweening::Gradient first{};
    Tweening::Gradient second{};

    std::size_t pos;

    if (!vgradient.empty() && (pos = vgradient.find(":")) != std::string::npos) {
        first.start = std::stoul(vgradient.substr(0, pos), nullptr, 16);
        first.end = std::stoul(vgradient.substr(pos + 1), nullptr, 16);

        first.type = Tweening::GradientType::Linear;
        first.orientation = Tweening::Orientation::Vertical;
    }

    if (!hgradient.empty() && (pos = hgradient.find(":")) != std::string::npos) {
        // when both v and h gradients are specified, use second for vertical
        Tweening::Gradient &dest = vgradient.empty() ? first : second;

        dest.start = std::stoul(hgradient.substr(0, pos), nullptr, 16);
        dest.end = std::stoul(hgradient.substr(pos + 1), nullptr, 16);

        dest.type = Tweening::GradientType::Linear;
        dest.orientation = Tweening::Orientation::Horizontal;
    }

    if (!rgradient.empty() && (pos = rgradient.find(":")) != std::string::npos) {
        first.start = std::stoul(rgradient.substr(0, pos), nullptr, 16);
        first.end = std::stoul(rgradient.substr(pos + 1), nullptr, 16);

        first.type = Tweening::GradientType::Radial;
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
