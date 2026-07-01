# Installs every dependency listed in libraries.txt via arduino-cli, so every
# developer machine and CI end up with identical library versions.
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$librariesFile = Join-Path $repoRoot "libraries.txt"

Get-Content $librariesFile | ForEach-Object {
    $line = ($_ -replace '#.*', '').Trim()
    if ($line.Length -eq 0) { return }
    Write-Host "Installing: $line"
    arduino-cli lib install "$line"
}

# LVGL's internal config header search (lv_conf_internal.h) looks for
# lv_conf.h next to the lvgl library folder itself, not in the sketch -- so
# our pinned copy (src/lv_conf.h) has to be placed there after every install.
$userDir = arduino-cli config get directories.user
Write-Host "Placing lv_conf.h in $userDir\libraries\"
Copy-Item (Join-Path $repoRoot "src\lv_conf.h") (Join-Path $userDir "libraries\lv_conf.h") -Force
