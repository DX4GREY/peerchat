param(
    [string]$BuildDir = "build/windows-release",
    [string]$OutputDir = "dist/windows",
    [string]$QtBinDir = "C:\Qt\6.6.2\msvc2022_64\bin"
)

Write-Host "==> Packaging PeerChat for Windows from $BuildDir..."

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$peerChatExe = if (Test-Path "$BuildDir/apps/client/PeerChat.exe") { "$BuildDir/apps/client/PeerChat.exe" } else { "$BuildDir/apps/client/Release/PeerChat.exe" }
$relayExe = if (Test-Path "$BuildDir/services/relay/peerchat-relay.exe") { "$BuildDir/services/relay/peerchat-relay.exe" } else { "$BuildDir/services/relay/Release/peerchat-relay.exe" }

if (Test-Path $peerChatExe) { Copy-Item $peerChatExe "$OutputDir/" }
if (Test-Path $relayExe) { Copy-Item $relayExe "$OutputDir/" }

if (Test-Path "$QtBinDir/windeployqt.exe") {
    Write-Host "==> Deploying Qt runtime DLLs..."
    & "$QtBinDir/windeployqt.exe" "$OutputDir/PeerChat.exe" --qmldir apps/client/qml
}

Compress-Archive -Path "$OutputDir/*" -DestinationPath "PeerChat-win64.zip" -Force
Get-FileHash -Algorithm SHA256 "PeerChat-win64.zip" | Out-File "PeerChat-win64.zip.sha256"

Write-Host "==> Windows Packaging complete: PeerChat-win64.zip"
