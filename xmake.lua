add_rules("mode.debug", "mode.release")

add_requires("pkgconfig::notcurses++")
target("ncfig")
    set_kind("binary")
    set_languages("cxx20")
    add_files("src/ncfig.cpp", "src/figlet.cppm")
    add_packages("pkgconfig::notcurses++")
