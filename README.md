# ncfig

![ncfig Logo](/logo.png "ncfig. This was rendered using the font univers.flf")

Renders text to the terminal using beautiful [FIGlet](https://www.figlet.org/) fonts, optionally with true color gradients.

Powered by [Notcurses](https://github.com/dankamongmen/notcurses).

## Usage

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

Gradients are specified as two hex colors separated by a colon:

```
ncfig -f fonts/banner.flf -v 0xff0000:0x0000ff "Hello"
```

Vertical and horizontal gradients can be combined (colors are added channel-wise). Radial gradients are mutually exclusive with the others.

## Getting Fonts

I found these sources for FIGlet fonts, two of them on GitHub:

 * The [font database](https://www.figlet.org/cgi-bin/fontdb.cgi) on the FIGlet site allows to browse and preview the fonts.
 * Claudio Matsuoka's repository [figlet-fonts](https://github.com/cmatsuoka/figlet-fonts) has everything the FIGlet font database has plus a lot of extra fonts, easily to be cloned to a local repository. Some of the fonts were automatically generated from old X11 fonts and C64 demo fonts, in these cases the quality is not always optimal.
 * The repository [github.com/xero/figlet-fonts](https://github.com/xero/figlet-fonts) has a huge collection of fonts in a huge single directory, some to be found on the previous two locations, some new.  It is easy to get an overview of the fonts thanks to the [examples page](https://github.com/xero/figlet-fonts/blob/main/Examples.md).

The FIGlet site has a FTP server as well, but as FTP is so very dead I assume you don't have a client for that.

## Building

Requires [xmake](https://xmake.io) and a recent C++ compiler.

```sh
xmake
```

You'll find the generated binary in `build/linux/x86_64/release/ncfig`.

Debug builds:

```sh
xmake f -m debug && xmake
```

In this case, the executable is dropped as `build/linux/x86_64/debug/ncfig`.

### Dependencies

- [Notcurses](https://github.com/dankamongmen/notcurses) (`notcurses++`) — discoverable via `pkg-config`
- GCC 16 or Clang 21 - I couldn't get Clang 22 to compile this.
