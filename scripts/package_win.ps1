# Windows packaging script
$ErrorActionPreference = "Stop"

Write-Host "Packaging Windows build..."

if (!(Test-Path "build/Release/unitml.exe")) {
    Write-Error "Build not found. Run cmake build first."
    exit 1
}

$outDir = "package-win"
Remove-Item -Recurse -Force $outDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $outDir | Out-Null

Copy-Item "build/Release/unitml.exe" $outDir/
Copy-Item "README.md" $outDir/

# Try to bundle WebView2Loader.dll if available
$wv2LoaderGlob = "C:\Program Files (x86)\Microsoft\EdgeWebView\Application\*\WebView2Loader.dll"
$loaderPath = Get-Item $wv2LoaderGlob -ErrorAction SilentlyContinue | Select-Object -First 1

if ($loaderPath) {
    Copy-Item $loaderPath.FullName $outDir/
    Write-Host "Bundled WebView2Loader.dll from EdgeWebView runtime"
} else {
    # Fallback to local archive if present
    if (Test-Path "webview2loader.zip") {
        $tmpDir = Join-Path $env:TEMP ("wv2_" + [System.Guid]::NewGuid().ToString())
        New-Item -ItemType Directory -Path $tmpDir | Out-Null
        try {
            Expand-Archive -Path "webview2loader.zip" -DestinationPath $tmpDir -Force
            $dll = Get-ChildItem -Path $tmpDir -Filter WebView2Loader.dll -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($dll) {
                Copy-Item $dll.FullName $outDir/
                Write-Host "Bundled WebView2Loader.dll from webview2loader.zip"
            } else {
                Write-Warning "webview2loader.zip did not contain WebView2Loader.dll"
            }
        } finally {
            Remove-Item -Recurse -Force $tmpDir -ErrorAction SilentlyContinue
        }
    } else {
        Write-Warning "WebView2Loader.dll not found. Users must install WebView2 Runtime."
        @"
WebView2 Runtime required. Download from:
https://developer.microsoft.com/microsoft-edge/webview2/#download-section
"@ | Out-File "$outDir/WEBVIEW2_REQUIRED.txt"
    }
}

Compress-Archive -Path "$outDir/*" -DestinationPath "unitml-win-x64.zip" -Force
Write-Host "Created unitml-win-x64.zip"

# Compute SHA256
$hash = (Get-FileHash "unitml-win-x64.zip" -Algorithm SHA256).Hash
Write-Host "SHA256: $hash"
