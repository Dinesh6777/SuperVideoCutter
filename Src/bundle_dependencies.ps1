$PortableDir = "$PSScriptRoot\..\PortableApp"

# Create target directories
$x64Dir = "$PortableDir\SuperVideoCutter_x64"
$x86Dir = "$PortableDir\SuperVideoCutter_x86"
If (-not (Test-Path $x64Dir)) { New-Item -ItemType Directory -Force -Path $x64Dir }
If (-not (Test-Path $x86Dir)) { New-Item -ItemType Directory -Force -Path $x86Dir }

Add-Type -AssemblyName System.IO.Compression.FileSystem

# Robust Download Helper with User-Agent to handle mirrors and redirects
Function Download-File($url, $dest) {
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.SecurityProtocolType]::Tls12
    $wc = New-Object System.Net.WebClient
    $wc.Headers.Add("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36")
    $wc.DownloadFile($url, $dest)
}

# Function to safely download and extract
Function Install-Dependencies($vlcUrl, $ffmpegUrl, $targetDir, $arch) {
    Write-Host "Checking dependencies for $arch in $targetDir..." -ForegroundColor Yellow
    
    $vlcDll = "$targetDir\libvlc.dll"
    $vlcCore = "$targetDir\libvlccore.dll"
    $ffmpegExe = "$targetDir\ffmpeg.exe"
    $ffprobeExe = "$targetDir\ffprobe.exe"
    
    $needVlc = (-not (Test-Path $vlcDll)) -or (-not (Test-Path $vlcCore)) -or (-not (Test-Path "$targetDir\plugins"))
    $needFfmpeg = (-not (Test-Path $ffmpegExe)) -or (-not (Test-Path $ffprobeExe))
    
    If ($needVlc) {
        Write-Host "Downloading VLC for $arch..." -ForegroundColor Cyan
        $tempVlc = "$env:TEMP\vlc_$arch.zip"
        Download-File -url $vlcUrl -dest $tempVlc
        
        Write-Host "Extracting VLC for $arch..." -ForegroundColor Green
        $zip = [System.IO.Compression.ZipFile]::OpenRead($tempVlc)
        ForEach ($e in $zip.Entries) {
            If ($e.Name -eq 'libvlc.dll' -or $e.Name -eq 'libvlccore.dll') {
                [System.IO.Compression.ZipFileExtensions]::ExtractToFile($e, "$targetDir\" + $e.Name, $true)
            } ElseIf ($e.FullName -match 'plugins/.*\.dll$') {
                $subPath = $e.FullName.Substring($e.FullName.IndexOf('plugins/'))
                $dest = [System.IO.Path]::Combine($targetDir, $subPath)
                $parent = Split-Path $dest
                If (-not (Test-Path $parent)) { New-Item -ItemType Directory -Force -Path $parent }
                [System.IO.Compression.ZipFileExtensions]::ExtractToFile($e, $dest, $true)
            }
        }
        $zip.Dispose()
        Remove-Item $tempVlc -Force
    } Else {
        Write-Host "VLC for $arch is already bundled." -ForegroundColor Gray
    }
    
    If ($needFfmpeg) {
        Write-Host "Downloading FFmpeg for $arch..." -ForegroundColor Cyan
        $tempFf = "$env:TEMP\ff_$arch.zip"
        Download-File -url $ffmpegUrl -dest $tempFf
        
        Write-Host "Extracting FFmpeg for $arch..." -ForegroundColor Green
        $zip = [System.IO.Compression.ZipFile]::OpenRead($tempFf)
        ForEach ($e in $zip.Entries) {
            If ($e.Name -eq 'ffmpeg.exe' -or $e.Name -eq 'ffprobe.exe') {
                [System.IO.Compression.ZipFileExtensions]::ExtractToFile($e, "$targetDir\" + $e.Name, $true)
            }
        }
        $zip.Dispose()
        Remove-Item $tempFf -Force
    } Else {
        Write-Host "FFmpeg for $arch is already bundled." -ForegroundColor Gray
    }
}

# Run for x64 (using download.videolan.org directly)
Install-Dependencies `
    -vlcUrl "https://download.videolan.org/pub/videolan/vlc/3.0.20/win64/vlc-3.0.20-win64.zip" `
    -ffmpegUrl "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip" `
    -targetDir $x64Dir `
    -arch "x64"

# Run for x86 (using download.videolan.org directly)
Install-Dependencies `
    -vlcUrl "https://download.videolan.org/pub/videolan/vlc/3.0.20/win32/vlc-3.0.20-win32.zip" `
    -ffmpegUrl "https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip" `
    -targetDir $x86Dir `
    -arch "x86"

# Create zip packages for C++ embedded resources
Write-Host "Creating embedded resource zip archives..." -ForegroundColor Yellow
$zip64 = "$PSScriptRoot\dependencies_x64.zip"
$zip86 = "$PSScriptRoot\dependencies_x86.zip"

If (-not (Test-Path $zip64)) {
    Write-Host "Compressing x64 dependencies zip..." -ForegroundColor Green
    Compress-Archive -Path "$x64Dir\*" -DestinationPath $zip64 -Force
} else {
    Write-Host "x64 dependencies zip already exists." -ForegroundColor Gray
}

If (-not (Test-Path $zip86)) {
    Write-Host "Compressing x86 dependencies zip..." -ForegroundColor Green
    Compress-Archive -Path "$x86Dir\*" -DestinationPath $zip86 -Force
} else {
    Write-Host "x86 dependencies zip already exists." -ForegroundColor Gray
}

Write-Host "Bundling completed successfully!" -ForegroundColor Green
