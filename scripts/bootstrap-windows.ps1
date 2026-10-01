param([switch]$DownloadOnly)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$toolDir = Join-Path $root '.tools'
New-Item -ItemType Directory -Force $toolDir | Out-Null
$packages = @(
    @{
        Name = 'llvm-mingw-20260922-ucrt-x86_64'; Archive = 'llvm-mingw-20260922-ucrt-x86_64.zip'
        Url = 'https://github.com/mstorsjo/llvm-mingw/releases/download/20260922/llvm-mingw-20260922-ucrt-x86_64.zip'
        Hash = 'e3ad77d117a4bea19a7a3b333341824d79a5a371004a10e25b8504e7b3047666'
        Destination = $toolDir
    },
    @{
        Name = 'ninja'; Archive = 'ninja-win-1.13.2.zip'
        Url = 'https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-win.zip'
        Hash = '07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65'
        Destination = (Join-Path $toolDir 'ninja')
    }
)
foreach ($package in $packages) {
    $archive = Join-Path $toolDir $package.Archive
    if (!(Test-Path -LiteralPath $archive)) {
        Write-Host "Downloading $($package.Name)..."
        & curl.exe --fail --location --retry 3 --output $archive $package.Url
        if ($LASTEXITCODE -ne 0) { throw "Download failed: $($package.Url)" }
    }
    if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $package.Hash) {
        throw "Checksum mismatch: $archive. Remove this archive and retry."
    }
    if (!$DownloadOnly) {
        $marker = Join-Path (Join-Path $toolDir $package.Name) '.installed'
        if (!(Test-Path -LiteralPath $marker)) {
            Write-Host "Extracting $($package.Name)..."
            Expand-Archive -LiteralPath $archive -DestinationPath $package.Destination -Force
            New-Item -ItemType File -Path $marker -Force | Out-Null
        }
    }
}
Write-Host 'Ready. Run: cmake --preset windows-debug'
