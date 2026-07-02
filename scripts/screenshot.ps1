# Requests a screenshot from the Socket device over serial and saves a PNG.
# See src/ui/ScreenshotService.h for the device-side protocol.
#
# Usage:
#   ./scripts/screenshot.ps1                  # COM3 -> screenshots/screenshot-<timestamp>.png
#   ./scripts/screenshot.ps1 -Port COM5 -OutFile ui.png
#
# Screenshots accumulate in screenshots/ with unique timestamped names --
# nothing is ever overwritten or cleaned up automatically; delete manually
# when no longer needed.
#
# Ordinary log lines received during the transfer are echoed to the console,
# so this doubles as a serial monitor while it waits -- no special client
# needed to see debug output and screenshots side by side.
param(
    [string]$Port = 'COM3',
    [string]$OutFile = '',
    [int]$Baud = 115200,
    [int]$TimeoutSec = 60
)

if (-not $OutFile) {
    $dir = Join-Path $PSScriptRoot '..\screenshots'
    if (-not (Test-Path $dir)) { $null = New-Item -ItemType Directory $dir }
    $OutFile = Join-Path $dir "screenshot-$(Get-Date -Format 'yyyyMMdd-HHmmss').png"
}

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$sp = New-Object System.IO.Ports.SerialPort $Port, $Baud, 'None', 8, 'One'
$sp.ReadTimeout = 2000
$sp.NewLine = "`n"
# CRITICAL: leave DTR and RTS both DEASSERTED (the defaults). The board's
# auto-reset circuit resets the chip when RTS is high while DTR is low
# (rebooting the board and destroying the very screen state being
# captured), and resets it INTO THE BOOTLOADER ("waiting for download",
# needs another reset to recover) when DTR is high while RTS is low during
# that reset. Windows applies the lines non-atomically during Open(), so
# any asserted combination can transiently hit one of those states.
# With both never asserted, no ordering can trigger either. Host->device
# data flows fine without DTR; the SNAP retry below covers the first
# moments while the port settles. Verified: 3 consecutive opens, no
# reboots, all answered.
$sp.Open()
try {
    $sp.DiscardInBuffer()
    $sp.Write("SNAP`n")

    $rleStream = New-Object System.IO.MemoryStream
    $width = 0; $height = 0
    $inFrame = $false
    $expectedBytes = -1
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    $nextRetry = (Get-Date).AddSeconds(4)

    while ((Get-Date) -lt $deadline) {
        # Opening the port can reboot the board (the DTR/RTS auto-reset
        # circuit can fire on the open transition), which eats the first
        # SNAP while the firmware boots. Re-send until the device answers.
        if (-not $inFrame -and (Get-Date) -gt $nextRetry) {
            $sp.Write("SNAP`n")
            $nextRetry = (Get-Date).AddSeconds(4)
        }
        try { $line = $sp.ReadLine() } catch [TimeoutException] { continue }
        if ($line.StartsWith('SS:')) {
            $body = $line.Substring(3).TrimEnd()
            if ($body.StartsWith('BEGIN')) {
                $parts = $body.Split(' ')
                $width = [int]$parts[1]; $height = [int]$parts[2]
                $inFrame = $true
                Write-Host "receiving ${width}x${height} screenshot..."
            } elseif ($body.StartsWith('END')) {
                $expectedBytes = [int]$body.Split(' ')[1]
                break
            } elseif ($body.StartsWith('ERR')) {
                throw "device reported: $body"
            } elseif ($inFrame -and $body.Length -gt 0) {
                $bytes = [Convert]::FromBase64String($body)
                $rleStream.Write($bytes, 0, $bytes.Length)
            }
        } else {
            Write-Host $line   # pass ordinary device logs through
        }
    }

    if (-not $inFrame) { throw "no screenshot data received within ${TimeoutSec}s (is the firmware's ScreenshotService flashed?)" }
    if ($expectedBytes -lt 0) { throw 'transfer did not complete (no SS:END)' }
    if ($rleStream.Length -ne $expectedBytes) {
        Write-Warning "byte count mismatch: got $($rleStream.Length), device sent $expectedBytes -- image may be corrupt"
    }

    # Decode RLE ([count:u8][pixel:u16le] pairs, RGB565) into 32bpp ARGB.
    $rle = $rleStream.ToArray()
    $pixels = New-Object int[] ($width * $height)
    $pi = 0
    for ($i = 0; $i + 3 -le $rle.Length; $i += 3) {
        $count = $rle[$i]
        # [int] cast is load-bearing: PowerShell's -shl keeps the byte type
        # of its left operand, so without it the high byte wraps to 0 and
        # every color collapses into blue shades.
        $p16 = $rle[$i + 1] -bor ([int]$rle[$i + 2] -shl 8)
        # RGB565 -> RGB888 with rounding-friendly bit replication
        $r = ($p16 -shr 11) -band 0x1F; $r = ($r -shl 3) -bor ($r -shr 2)
        $g = ($p16 -shr 5)  -band 0x3F; $g = ($g -shl 2) -bor ($g -shr 4)
        $b = $p16 -band 0x1F;           $b = ($b -shl 3) -bor ($b -shr 2)
        $argb = [int](0xFF000000 -bor ($r -shl 16) -bor ($g -shl 8) -bor $b)
        $end = [Math]::Min($pi + $count, $pixels.Length)
        if ($pi -lt $end) { [System.Array]::Fill($pixels, $argb, $pi, $end - $pi) }
        $pi = $end
    }
    if ($pi -ne $pixels.Length) {
        Write-Warning "decoded $pi of $($pixels.Length) pixels -- image may be truncated"
    }

    $bmp = New-Object System.Drawing.Bitmap($width, $height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $rect = New-Object System.Drawing.Rectangle(0, 0, $width, $height)
    $data = $bmp.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::WriteOnly, $bmp.PixelFormat)
    [System.Runtime.InteropServices.Marshal]::Copy($pixels, 0, $data.Scan0, $pixels.Length)
    $bmp.UnlockBits($data)
    if (-not [System.IO.Path]::IsPathRooted($OutFile)) {
        $OutFile = Join-Path (Get-Location) $OutFile
    }
    $bmp.Save($OutFile, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Host "saved $OutFile ($($rleStream.Length) RLE bytes over the wire, $($width * $height * 2) raw)"
} finally {
    $sp.Close()
}
