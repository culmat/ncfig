#include <ncpp/NotCurses.hh>
#include <ncpp/Plane.hh>
#include <iostream>

import figlet;

int main(int argc, char **argv) {
    notcurses_options options{
        .flags = NCOPTION_SUPPRESS_BANNERS | NCOPTION_CLI_MODE
    };

    auto nc = ncpp::NotCurses(options);
    ncpp::Plane* std_plane = nc.get_stdplane();

    FigletFont font;
    if (!font.loadFromFile("standard.flf")) {
        std::cerr << "Failed to parse font file.\n";
        return EXIT_FAILURE;
    }

    // Render using the current cursor position
    std_plane->putstr("\n");
    font.render(std_plane, argc > 0 ? argv[1] : "HELLO NOTCURSES!");
    nc.render();

    return EXIT_SUCCESS;
}
