#!/usr/bin/env bash
set -euo pipefail

echo "Packaging macOS build..."

if [[ ! -f "build/unitml.app/Contents/MacOS/unitml" ]]; then
    echo "Build not found. Run cmake build first."
    exit 1
fi

OUT_DIR="package-mac"
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

APP_BUNDLE="build/unitml.app"
cp -R "$APP_BUNDLE" "$OUT_DIR/"
cp README.md "$OUT_DIR/" 2>/dev/null || true

ZIP_NAME="unitml-macos.zip"
rm -f "$ZIP_NAME"
(
  cd "$OUT_DIR"
  zip -r "../$ZIP_NAME" * >/dev/null
)
echo "Created $ZIP_NAME"

# Compute SHA256 (macOS uses shasum)
if command -v shasum >/dev/null 2>&1; then
  shasum -a 256 "$ZIP_NAME"
elif command -v sha256sum >/dev/null 2>&1; then
  sha256sum "$ZIP_NAME"
fi
