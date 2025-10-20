# UniTML :: Cross-Platform Single-Binary Launcher

**Version:** 1.2  
**License:** MIT

## Features

- **Zero-config auto-discovery**: Detects frontend (HTML) and backend (Node.js) from folder structure
- **Single binary**: No external config files required at runtime
- **Native webview**: Uses system WebView (WKWebView/WebView2/WebKitGTK)
- **Single instance**: Only one app runs; deep links focus existing window
- **Protocol handler**: `unitml://` deep linking support
- **Auto health check**: Waits for backend readiness before opening browser view
- **Clean shutdown**: Gracefully terminates child processes

## How It Works

UniTML scans its directory for:

1. **Backend** in `backend/`, `server/`, or `api/`:
   - `package.json` with `scripts.start` → runs `npm run start`
   - `server.js` → runs `node server.js`
   - Sets `PORT=5173` if not already set

2. **Frontend** (first match wins):
   - `frontend/dist/index.html`
   - `frontend/public/index.html`
   - `frontend/index.html`
   - `public/index.html`
   - `index.html`

3. **Entry point**:
   - Frontend file exists → opens `file://` path immediately
   - Backend only → waits for HTTP 200 from `http://127.0.0.1:<PORT>`
   - Both → opens file:// immediately, spawns backend in parallel

## Folder Conventions
```
your-app/
  unitml(.exe)           # This binary
  backend/
    package.json         # Must have "scripts": {"start": "..."}
    server.js
  frontend/
    index.html           # Or dist/index.html
```

## Deep Link Setup

### Windows
Run `installers/windows/registry-deeplink.reg` (edit path to your `.exe` first):
```reg
Windows Registry Editor Version 5.00

[HKEY_CLASSES_ROOT\unitml]
@="URL:UniTML Protocol"
"URL Protocol"=""

[HKEY_CLASSES_ROOT\unitml\shell\open\command]
@="\"C:\\path\\to\\unitml.exe\" \"%1\""
```

### macOS
Merge `installers/macos/Info.plist.fragment.xml` into your `.app/Contents/Info.plist`:
```xml
<key>CFBundleURLTypes</key>
<array>
  <dict>
    <key>CFBundleURLName</key>
    <string>com.unitml.protocol</string>
    <key>CFBundleURLSchemes</key>
    <array>
      <string>unitml</string>
    </array>
  </dict>
</array>
```

### Linux
Install `installers/linux/unitml.desktop` to `~/.local/share/applications/`:
```desktop
[Desktop Entry]
Name=UniTML
Exec=/path/to/unitml %u
Type=Application
MimeType=x-scheme-handler/unitml;
```
Then run: `xdg-mime default unitml.desktop x-scheme-handler/unitml`

## Runtime Dependencies

- **Windows**: WebView2 Runtime (auto-downloads if missing via `WebView2Loader.dll`)
- **macOS**: None (system WebKit)
- **Linux**: `webkit2gtk-4.0`, `libcurl`
```bash
  # Debian/Ubuntu
  sudo apt install libwebkit2gtk-4.0-37 libcurl4
  
  # Fedora
  sudo dnf install webkit2gtk3 libcurl
```

## Building
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

**Per-OS specifics:**
- Windows: Requires Visual Studio 2019+ or MinGW with C11 support
- macOS: Xcode command-line tools
- Linux: GCC/Clang + pkg-config + webkit2gtk-4.0-dev + libcurl4-openssl-dev

## Packaging
```bash
# Windows
pwsh scripts/package_win.ps1

# macOS
bash scripts/package_mac.sh

# Linux
bash scripts/package_linux.sh
```

Outputs:
- `unitml-win-x64.zip`
- `unitml-macos.zip`
- `unitml-linux.tar.gz`

## Troubleshooting

**Port already in use:**  
UniTML defaults to `PORT=5173`. Set `PORT` env var before running:
```bash
PORT=3000 ./unitml
```

**WebView2 missing (Windows):**  
Download from: https://developer.microsoft.com/microsoft-edge/webview2/#download-section  
Or let `WebView2Loader.dll` bootstrap it automatically.

**Backend not starting:**  
Check `npm` and `node` are in PATH. Ensure `package.json` has `scripts.start`.

**Deep link not working:**  
Verify protocol registration. Test with: `unitml://test?foo=bar`

## Security

- DevTools disabled in production
- Webview sandboxed
- JS bridge exposes only `nativePing()`
- No filesystem access beyond explicit bindings

## Verification

SHA256 checksums for releases:
```
# Example
sha256sum unitml-win-x64.zip
sha256sum unitml-macos.zip
sha256sum unitml-linux.tar.gz
```

## Third-Party Components

- **webview**: https://github.com/zserge/webview (commit: vendored in `third_party/`)
- **libcurl**: https://curl.se/libcurl/

---

**Questions?** Open an issue at your repository.