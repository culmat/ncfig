add_rules("mode.debug", "mode.release")

-- System dependencies. They are optional at the package level so that a
-- missing one doesn't abort with xmake's terse "must be installed!", but the
-- "preflight" rule below turns a missing one into a diagnosis with fixes.
add_requires("pkgconfig::notcurses++", {optional = true})

-- Checks the compiler and every package a target uses and, on a problem, reports
-- why and how to fix it. Install hints live in xmake/preflight.lua.
rule("preflight")
    on_config(function (target)
        import("preflight", {rootdir = path.join(os.projectdir(), "xmake")})
        preflight.check(target)
    end)
rule_end()

target("ncfig")
    set_kind("binary")
    set_languages("cxx20")
    add_rules("preflight")
    add_files("src/ncfig.cpp", "src/*.cppm")
    add_packages("pkgconfig::notcurses++")

target("ncmarquee")
    set_kind("binary")
    set_languages("cxx20")
    add_rules("preflight")
    add_files("src/ncmarquee.cpp", "src/*.cppm")
    add_packages("pkgconfig::notcurses++")
