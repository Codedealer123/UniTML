#!/usr/bin/env bash
set -euo pipefail

echo "Packaging Linux build..."

if [[ ! -f "build/unitml" ]]; then
    echo "Build not found. Run cmake build first."
    exit 1
fi

OUT_DIR="package-linux"
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

cp build/unitml "$OUT_DIR/"
cp README.md "$OUT_DIR/"

cat > "$OUT_DIR/DEPENDENCIES.txt" << 'EOF'
Runtime Dependencies:
- WebKitGTK (webkit2gtk) runtime
- libcurl

Install on Ubuntu/Debian:
  # Ubuntu 22.04 / Debian 12 (4.0 slot)
  sudo apt install libwebkit2gtk-4.0-37 libcurl4

  # Ubuntu 24.04 (4.1 slot)
  sudo apt install libwebkit2gtk-4.1-0 libcurl4

Install on Fedora:
  sudo dnf install webkit2gtk3 libcurl

Install on Arch:
  sudo pacman -S webkit2gtk libcurl-gnutls
EOF

(
  cd "$OUT_DIR"
  tar czf ../unitml-linux.tar.gz *
)

echo "Created unitml-linux.tar.gz"
if command -v sha256sum >/dev/null 2>&1; then
  sha256sum unitml-linux.tar.gz
elif command -v shasum >/dev/null 2>&1; then
  shasum -a 256 unitml-linux.tar.gz
fi
