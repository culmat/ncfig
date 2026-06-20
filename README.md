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
ncmarquee -f <font> [-v <gradient>] [-h <gradient>] [-r <gradient>] [-s <ms>] [text]
```

| Option | Description |
|--------|-------------|
| `-f <font>` | Path to a FIGlet font file (`.flf`) |
| `-v <gradient>` | Vertical color gradient |
| `-h <gradient>` | Horizontal color gradient |
| `-r <gradient>` | Radial color gradient |
| `-s <ms>` | Scroll speed in milliseconds (default: 50) |
| `-t` | Apply gradient to text dimensions (default: apply to the visible area of the text) |
| `-c` | Circular mode (text is directly followed by itself) |
| `text` | Text to render (default: `HELLO NOTCURSES!`) |

**Examples:**

```
# Default speed (50ms)
ncmarquee -f fonts/standard.flf "MARQUEE"

# Faster animation (20ms delay)
ncmarquee -f fonts/standard.flf -s 20 "HELLO WORLD"

# With gradient
ncmarquee -f fonts/banner.flf -h 0xff0000:0x0000ff -s 30 "NOTCURSES"
```

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

### Debug builds:

```sh
xmake f -m debug && xmake
```

In this case, the executables are in `build/linux/x86_64/debug/`.

### Dependencies

- [Notcurses](https://github.com/dankamongmen/notcurses) (`notcurses++`) — discoverable via `pkg-config`
- GCC 16 or Clang 21 - I couldn't get Clang 22 to compile this.
