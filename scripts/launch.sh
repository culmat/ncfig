#!/usr/bin/env bash
#
# Try ncfig without installing anything:
#
#   bash <(curl -L https://raw.githubusercontent.com/jaydg/ncfig/main/scripts/launch.sh)
#
# Downloads the latest release for this machine into a temporary directory and
# runs it from there. Without arguments it plays a short demo. Otherwise the
# first argument picks the tool and the rest are passed on, e.g.
#
#   ... launch.sh) ncfig -f slant -v 0xff0000:0x0000ff "Hello"
#   ... launch.sh) ncmarquee -f banner -w 3 -c "LOOP"
#
# Nothing is installed: the files live in ~/.cache/ncfig/launch (or under
# $XDG_CACHE_HOME) and are reused until that directory is deleted, which is
# also how to get a newer release. To install for good, see install.sh next
# to this script.

set -euo pipefail

repo="${NCFIG_REPO:-jaydg/ncfig}"   # override to use another fork's releases
cache="${XDG_CACHE_HOME:-$HOME/.cache}/ncfig"
dir="$cache/launch"
launch="https://raw.githubusercontent.com/$repo/main/scripts/launch.sh"

if [ ! -x "$dir/ncfig" ] || [ ! -x "$dir/ncmarquee" ] || [ ! -d "$dir/fonts" ]; then
    case "$(uname -m)" in
        x86_64|amd64)  arch="x86_64" ;;
        arm64|aarch64) arch="aarch64" ;;
        *) echo "Unsupported CPU architecture: $(uname -m)" >&2; exit 2 ;;
    esac
    case "$(uname -s)" in
        Linux)  os="linux" ;;
        Darwin) os="macos" ;;
        *) echo "Unsupported operating system: $(uname -s)" >&2; exit 2 ;;
    esac

    url="https://github.com/$repo/releases/latest/download/ncfig-$arch-$os.tar.gz"
    echo "Downloading $url ..." >&2
    mkdir -p "$cache"
    tmp="$(mktemp -d "$cache/download.XXXXXX")"
    if ! curl -fsSL "$url" | tar -C "$tmp" -xz; then
        rm -rf "$tmp"
        echo "Download failed. Is there a release for $arch-$os at https://github.com/$repo/releases ?" >&2
        exit 1
    fi
    rm -rf "$dir"
    mv "$tmp" "$dir"
fi

# the tools look here for fonts given by name, e.g. -f banner
export NCFIG_FONTDIR="$dir/fonts"

tool="ncfig"
case "${1:-}" in
    ncfig|ncmarquee) tool="$1"; shift ;;
esac

if [ $# -eq 0 ] && [ "$tool" = "ncfig" ]; then
    # demo: a wavy, circular marquee; any key ends it
    "$dir/ncmarquee" -f banner -h 0xff0000:0x00ff00:0x0000ff -w 3 -c -s 30 "NCFIG"
    fonts=""
    for f in "$dir"/fonts/*.flf; do f="${f##*/}"; fonts="$fonts ${f%.flf}"; done
    cat >&2 <<HINT

That was ncmarquee. More to try (fonts:$fonts):

  bash <(curl -L $launch) ncfig -f slant -v 0xff0000:0x0000ff "Hello"
  bash <(curl -L $launch) ncmarquee -f big -r 0xffff00:0xff00ff -w 4 "WAVE"

Install it: bash <(curl -L https://raw.githubusercontent.com/$repo/main/scripts/install.sh)
HINT
    exit 0
fi

exec "$dir/$tool" "$@"
