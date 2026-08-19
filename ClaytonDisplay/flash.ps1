# flash.ps1 - flash ClaytonDisplay over the USB-C port.
#
# The USB CAN modem firmware owns the USB port (TinyUSB), so esptool cannot
# reset the chip into download mode by itself. This script opens the DEBUG
# COM port and sends the 'BT' command; the firmware then reboots into ROM
# download mode when the port is closed (see usb_modem.c). The script waits
# for the download-mode port and runs idf.py flash on it.
#
# Fallback if the touch fails (e.g. firmware crashed before USB init, or
# the display is in standby with USB off): wake the display by touching it,
# or hold the BOOT button and press RESET - then run this again.
#
# Usage:  .\flash.ps1            # flash
#         .\flash.ps1 -Monitor   # ... then attach idf.py monitor

param([switch]$Monitor)

$ErrorActionPreference = 'Continue'
Set-Location $PSScriptRoot

if (-not (Get-Command idf.py -ErrorAction SilentlyContinue)) {
    . C:\esp\v6.0\esp-idf\export.ps1 *> $null
}

# The BT command lives on the DEBUG/console port - the SECOND CDC interface
# (MI_02) of the TinyUSB device (VID 303A, not the USB-Serial-JTAG PID 1001;
# that PID means the chip is already in download mode / running old firmware,
# so no touch is needed). Using the debug port means the CAN tool can keep
# the modem port open while we flash.
$debugPorts = Get-CimInstance Win32_PnPEntity |
    Where-Object { $_.Name -match 'COM\d+' -and $_.DeviceID -match 'VID_303A' -and
                   $_.DeviceID -notmatch 'PID_1001' -and $_.DeviceID -match 'MI_02' } |
    ForEach-Object { [regex]::Match($_.Name, 'COM\d+').Value } |
    Sort-Object -Unique

foreach ($p in $debugPorts) {
    Write-Host "Sending BT command on debug port $p (reboots to download mode on close)..."
    try {
        $sp = New-Object System.IO.Ports.SerialPort $p, 115200
        $sp.DtrEnable = $true
        $sp.Open()
        Start-Sleep -Milliseconds 200
        $sp.Write("BT`r")
        Start-Sleep -Milliseconds 200
        $sp.Close()
    } catch {
        Write-Host "$p could not be opened ($($_.Exception.Message.Trim()))." -ForegroundColor Yellow
        Write-Host "If the display is asleep, wake it by touching the screen and retry;" -ForegroundColor Yellow
        Write-Host "or close the program holding the port." -ForegroundColor Yellow
        exit 1
    }
}
# Download mode enumerates on a DIFFERENT COM number than the app's CDC
# ports: PID 1001 (USB-Serial-JTAG, after BOOT button) or the ROM's USB-OTG
# CDC (after the BT command - any 303A PID that is not the app's 4002).
# Poll for it and hand it to esptool explicitly - its auto-detect gets
# confused by other serial devices.
$dlPort = $null
for ($i = 0; $i -lt 20 -and -not $dlPort; $i++) {
    Start-Sleep -Milliseconds 500
    $dl = Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue |
        Where-Object { $_.InstanceId -match 'VID_303A&PID_(?!4002)' -and $_.FriendlyName -match 'COM\d+' } |
        Select-Object -First 1
    if ($dl) { $dlPort = [regex]::Match($dl.FriendlyName, 'COM\d+').Value }
}

if ($dlPort) {
    # The port shows up in PnP before it can actually be opened (Windows
    # driver setup / security agents probe new ports for several seconds).
    # Poll until WE can open it - only then hand it to esptool.
    Write-Host "Download-mode port found: $dlPort - waiting until it can be opened..."
    $ready = $false
    for ($i = 0; $i -lt 30 -and -not $ready; $i++) {
        try {
            $sp = New-Object System.IO.Ports.SerialPort $dlPort
            $sp.Open(); $sp.Close()
            $ready = $true
        } catch { Start-Sleep -Seconds 1 }
    }
    if (-not $ready) { Write-Host "$dlPort never became available." -ForegroundColor Yellow }
    idf.py -p $dlPort flash
} else {
    Write-Host "No download-mode port detected - trying esptool auto-detect..."
    idf.py flash
}
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "Flash failed. Manual download mode: hold BOOT, press RESET," -ForegroundColor Yellow
    Write-Host "release BOOT, then run .\flash.ps1 again." -ForegroundColor Yellow
    exit 1
}

Write-Host "Flashed OK."
Write-Host "NOTE: if the display entered bootmode via the BOOT button (not the"
Write-Host "BT command), press RESET now to start the app."
if ($Monitor) { idf.py monitor }
