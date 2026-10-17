add_rules("mode.debug", "mode.release")

add_requires("pkgconfig::notcurses++", {optional = true})

target("figlet-core")
    set_kind("static")
    set_languages("cxx20")
    add_files("src/canvas.cppm", "src/tweening.cppm", "src/figlet.cppm", {public = true})
    add_headerfiles("src/canvas.cppm", "src/tweening.cppm", "src/figlet.cppm")

target("ncfig")
    set_kind("binary")
    set_languages("cxx20")
    add_deps("figlet-core")
    add_files("src/ncfig.cpp", "src/cmdline.cppm")
    add_packages("pkgconfig::notcurses++")

target("ncmarquee")
    set_kind("binary")
    set_languages("cxx20")
    add_deps("figlet-core")
    add_files("src/ncmarquee.cpp", "src/cmdline.cppm")
    add_packages("pkgconfig::notcurses++")

