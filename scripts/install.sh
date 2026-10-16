#!/usr/bin/env bash
#
# Install the latest ncfig release without compiling:
#
#   bash <(curl -L https://raw.githubusercontent.com/jaydg/ncfig/main/scripts/install.sh)
#
# Installs ncfig and ncmarquee into $PREFIX/bin and the standard FIGlet fonts
# into $PREFIX/share/ncfig/fonts. PREFIX defaults to ~/.local; set it to
# install elsewhere, e.g. PREFIX=/usr/local sudo -E bash <(curl ...).
# A specific version can be chosen with VERSION=v0.1.0, and NCFIG_REPO=owner/ncfig
# installs from another fork's releases.

set -euo pipefail

repo="${NCFIG_REPO:-jaydg/ncfig}"   # override to use another fork's releases
prefix="${PREFIX:-$HOME/.local}"
version="${VERSION:-latest}"

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

asset="ncfig-$arch-$os.tar.gz"
if [ "$version" = "latest" ]; then
    url="https://github.com/$repo/releases/latest/download/$asset"
else
    url="https://github.com/$repo/releases/download/$version/$asset"
fi

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

echo "Downloading $url ..."
if ! curl -fsSL "$url" | tar -C "$tmp" -xz; then
    echo "Download failed. Is there a release for $arch-$os at https://github.com/$repo/releases ?" >&2
    exit 1
fi

mkdir -p "$prefix/bin" "$prefix/share/ncfig/fonts"
install -m 755 "$tmp/ncfig" "$tmp/ncmarquee" "$prefix/bin/"
install -m 644 "$tmp"/fonts/*.flf "$prefix/share/ncfig/fonts/"

echo "Installed ncfig and ncmarquee to $prefix/bin, fonts to $prefix/share/ncfig/fonts."
case ":$PATH:" in
    *":$prefix/bin:"*) ;;
    *) echo "Note: $prefix/bin is not on your PATH. Add it, e.g.: export PATH=\"$prefix/bin:\$PATH\"" ;;
esac
echo 'Try: ncfig -f banner -v 0xff0000:0x0000ff "Hello"'
