#include <ncpp/NotCurses.hh>
#include <ncpp/Plane.hh>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using namespace std::string_view_literals;

import figlet;

// Source - https://stackoverflow.com/a/868894
// Posted by iain, modified by community. See post 'Timeline' for change history
// Retrieved 2026-06-18, License - CC BY-SA 3.0

class InputParser{
    public:
        InputParser (int &argc, char **argv){
            for (int i=1; i < argc; ++i)
                this->tokens.push_back(std::string(argv[i]));
        }

        const std::string& getCmdOption(const std::string &option) const{
            std::vector<std::string>::const_iterator itr;
            itr =  std::find(this->tokens.begin(), this->tokens.end(), option);
            if (itr != this->tokens.end() && ++itr != this->tokens.end()){
                return *itr;
            }
            static const std::string empty_string("");
            return empty_string;
        }

        bool cmdOptionExists(const std::string &option) const{
            return std::find(this->tokens.begin(), this->tokens.end(), option)
                   != this->tokens.end();
        }
    private:
        std::vector<std::string> tokens;
};

auto help = "Usage: figdemo -f <font> [-g <gradient>] [text]\n"
    "  -f <font>      : Specify the font file to use\n"
    "  -g <gradient>  : Specify the color gradient (FromColor:ToColor, e.g. 0xff0000:0x00ff00)\n"
    "  text           : The text to render\n"sv;

void print_help() {
    std::cout << help;
}

int main(int argc, char **argv) {
    InputParser opts(argc, argv);

    if (opts.cmdOptionExists("-h") || opts.cmdOptionExists("--help")) {
        print_help();
        return EXIT_SUCCESS;
    }

    std::string filename = opts.getCmdOption("-f");
    std::string gradient = opts.getCmdOption("-g");
    std::string text = argc % 2 == 0 ? argv[argc - 1] : "HELLO NOTCURSES!";

    if (filename.empty()) {
        print_help();
        return EXIT_FAILURE;
    }

    std::uint32_t cstart=0, cend=0;

    std::size_t pos;
    if (!gradient.empty() && (pos = gradient.find(":")) != std::string::npos) {
        cstart = std::stoul(gradient.substr(0, pos), nullptr, 16);
        cend = std::stoul(gradient.substr(pos + 1), nullptr, 16);
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
    font.render(std_plane, text, cstart, cend);
    nc.render();

    return EXIT_SUCCESS;
}
