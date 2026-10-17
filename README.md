# ncfig: NotCurses Tools for FIGlet Fonts

A collection of command-line tools for rendering beautiful [FIGlet](https://www.figlet.org/) fonts with [Notcurses](https://github.com/dankamongmen/notcurses), featuring true color gradient support.

![ncfig Logo](/logo.png "ncfig. This was rendered using the font univers.flf")

## Tools

### ncfig

Renders text to the terminal using FIGlet fonts.

**Usage:**

```
ncfig -f <font> [-v <gradient>] [-h <gradient>] [-r <gradient>] [text]
```

| Option | Description |
|--------|-------------|
| `-f <font>` | Path to a FIGlet font file (`.flf`) |
| `-v <gradient>` | Vertical color gradient |
| `-h <gradient>` | Horizontal color gradient |
| `-r <gradient>` | Radial color gradient |
| `text` | Text to render (default: `HELLO NOTCURSES!`) |

**Example:**

```
ncfig -f fonts/banner.flf -v 0xff0000:0x0000ff "Hello"
```

Vertical and horizontal gradients can be combined (colors are added channel-wise). Radial gradients are mutually exclusive with the others.

### ncmarquee

Creates a full-screen marquee animation with FIGlet text. The text floats from right to left, and when it has left the visible area entirely, it comes back in from the right. Press any key to exit.

**Usage:**

```
ncmarquee -f <font> [-v <gradient>] [-h <gradient>] [-r <gradient>] [-s <ms>] [-t] [-c] [-w <amplitude>] [text]
```

| Option | Description |
|--------|-------------|
| `-f <font>` | Path to a FIGlet font file (`.flf`) |
| `-v <gradient>` | Vertical color gradient |
| `-h <gradient>` | Horizontal color gradient |
| `-r <gradient>` | Radial color gradient |
| `-s <ms>` | Scroll speed in milliseconds (default: 50) |
| `-t` | Apply gradient and wave to text dimensions (default: apply to the visible area) |
| `-c` | Circular mode (text is directly followed by itself) |
| `-w <amplitude>` | Sine wave vertical movement with the given amplitude in rows |
| `text` | Text to render (default: `HELLO NOTCURSES!`) |

**Examples:**

```
# Default speed (50ms)
ncmarquee -f fonts/standard.flf "MARQUEE"

# Faster animation (20ms delay)
ncmarquee -f fonts/standard.flf -s 20 "HELLO WORLD"

# With gradient
ncmarquee -f fonts/banner.flf -h 0xff0000:0x0000ff -s 30 "NOTCURSES"

# Sine wave with 4-row amplitude
ncmarquee -f fonts/banner.flf -w 4 "WAVE"

# Circular sine wave — seamless across repeated instances
ncmarquee -f fonts/banner.flf -c -w 3 "LOOP"
```

#### Sine wave mode (`-w`)

The `-w <amplitude>` flag adds a vertical sine wave through the scrolling text. Each FIGlet character is displaced independently on the y-axis according to a continuous sine wave, so the text undulates as it scrolls. The amplitude is given in terminal rows.

The `-t` flag controls the wave's frame of reference:

- **`-t` off (default):** the wave is fixed in screen space — each screen column has a fixed y-offset at any moment in time, and the text scrolls through it. In circular mode (`-c`) this means adjacent instances share the same wave seamlessly, with no discontinuity at the wrap point.
- **`-t` on:** the wave is fixed to the text — each character carries its own phase based on its position in the string, so the wave pattern travels with the text as it scrolls.

Because terminal rows are discrete, the wave will always be a staircase of integer row steps between characters rather than a truly smooth curve. Larger amplitudes make individual steps proportionally smaller and less noticeable.

## Gradients

Gradients are specified as pairs of hexadecimal colors separated by a colon:

- `0xff0000` = Red
- `0x00ff00` = Green
- `0x0000ff` = Blue
- `0xffff00` = Yellow
- `0xff00ff` = Magenta
- `0x00ffff` = Cyan

Example: `0xff0000:0x0000ff` creates a red-to-blue gradient. Two colors are mandatory, but you can optionally add a third color as center color, e.g. e.g. 0xff0000:0x00ff00:0x0000ff for a red-to-green-to-blue gradient.

## Getting Fonts

I found these sources for FIGlet fonts, two of them on GitHub:

 * The [font database](https://www.figlet.org/cgi-bin/fontdb.cgi) on the FIGlet site allows to browse and preview the fonts.
 * Claudio Matsuoka's repository [figlet-fonts](https://github.com/cmatsuoka/figlet-fonts) has everything the FIGlet font database has plus a lot of extra fonts, easily to be cloned to a local repository. Some of the fonts were automatically generated from old X11 fonts and C64 demo fonts, in these cases the quality is not always optimal.
 * The repository [github.com/xero/figlet-fonts](https://github.com/xero/figlet-fonts) has a huge collection of fonts in a huge single directory, some to be found on the previous two locations, some new.  It is easy to get an overview of the fonts thanks to the [examples page](https://github.com/xero/figlet-fonts/blob/main/Examples.md).

The FIGlet site has a FTP server as well, but as FTP is so very dead I assume you don't have a client for that.

## Building

Requires [xmake](https://xmake.io) and a recent C++ compiler.

### Build all tools:

```sh
xmake
```

Binaries will be in `build/linux/x86_64/release/`:
- `ncfig` - Static FIGlet renderer
- `ncmarquee` - Full-screen marquee animation

### Build a specific tool:

```sh
xmake build ncfig
xmake build ncmarquee
```

### Build only the core library (zero external dependencies):

```sh
xmake build figlet-core
```

This compiles `libfiglet-core.a` with `FigletFont`, `TextCanvas`, and `Tweening`
without requiring Notcurses. You can integrate this target into other projects
using `includes(...)` and `add_deps("figlet-core")`.

### Debug builds:

```sh
xmake f -m debug && xmake
```

In this case, the executables are in `build/linux/x86_64/debug/`.

### Dependencies

- GCC 16 or Clang 21 - I couldn't get Clang 22 to compile this.
- [Notcurses](https://github.com/dankamongmen/notcurses) (`notcurses++`) — discoverable via `pkg-config` (required only for the `ncfig` and `ncmarquee` demo tools).

## Using the Core with Other Renderers

The core FIGlet engine (`FigletFont`, `Tweening`, and `TextCanvas`) is
completely independent of Notcurses or any specific rendering library.
It has zero external dependencies and only requires the C++20 standard
library.

The core renders into a generic `TextCanvas`, which is an in-memory 2D buffer of `Cell` structures:

```cpp
struct Cell {
    char ch = ' ';
    uint32_t fg = 0xFFFFFF; // 0x00RRGGBB format

    uint8_t r() const noexcept;
    uint8_t g() const noexcept;
    uint8_t b() const noexcept;
};
```

### 1. Rendering Text to `TextCanvas`

```cpp
import nctui.figlet;
import nctui.tweening;

FigletFont font;
font.loadFromFile("fonts/banner.flf");

// Define an optional color gradient
Tweening::Gradient grad{
    .type = Tweening::GradientType::Linear,
    .orientation = Tweening::Orientation::Horizontal,
    .start = 0xff0000,
    .end = 0x0000ff
};

// Render directly into a newly created canvas sized to fit the text:
TextCanvas canvas = font.render("HELLO", grad);
```

### 2. Using with STC (`simple_term_colors`)

[STC](https://github.com/illyigan/simple_term_colors) allows streaming ANSI
escape sequences directly to `std::cout` without requiring any curses
library or TTY initialization:

```cpp
#include "stc.hpp"
import nctui.figlet;

void printWithSTC(const TextCanvas& canvas)
{
    std::cout << stc::true_color;
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            const auto& cell = canvas(y, x);
            std::cout << stc::rgb_fg(cell.r(), cell.g(), cell.b()) << cell.ch;
        }
        std::cout << stc::reset << '\n';
    }
}
```

### 3. Using with ncurses (`WINDOW*`)

With classic `ncurses`, map the RGB color to an ncurses color pair and write cells using `mvwaddch`:

```cpp
#include <ncurses.h>
import nctui.figlet;

void blitToNcurses(WINDOW* win, const TextCanvas& canvas, int start_y = 0, int start_x = 0)
{
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            const auto& cell = canvas(y, x);
            int pair = mapRgbToPair(cell.fg); // Custom mapping to closest init_pair
            wattron(win, COLOR_PAIR(pair));
            mvwaddch(win, start_y + y, start_x + x, cell.ch);
            wattroff(win, COLOR_PAIR(pair));
        }
    }
    wrefresh(win);
}
```

### 4. Using with Notcurses (as in `ncfig`)

Blit the `TextCanvas` cells directly to an `ncpp::Plane`:

```cpp
#include <ncpp/Plane.hh>
import nctui.figlet;

void blitToPlane(ncpp::Plane* plane, const TextCanvas& canvas, int start_y = 0, int start_x = 0)
{
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            const auto& cell = canvas(y, x);
            plane->set_fg_rgb(cell.fg);
            plane->putc(start_y + y, start_x + x, cell.ch);
        }
    }
}
```

### 5. Viewport / Marquee Animations

For scrolling or sine wave marquee animations, use `renderToViewport` to
render each frame into a viewport-sized `TextCanvas`:

```cpp
TextCanvas canvas(screen_width, screen_height);
canvas.clear({' ', 0});

font.renderToViewport(
    canvas,
    y_pos,
    x_pos,
    text,
    first_gradient,
    second_gradient,
    screen_width,
    x_pos,               // offset for scrolling
    gradient_to_text,    // true: text space; false: screen space
    wave_amplitude,      // 0 for no wave
    wave_phase           // advancing angle for sine animation
);

// Then blit canvas to your target display (Notcurses, STC, ncurses, SDL, etc.)
```

