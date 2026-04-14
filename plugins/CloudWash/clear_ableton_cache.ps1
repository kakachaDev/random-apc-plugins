Write-Host "=== Ableton Live Plugin Cache Cleanup ===" -ForegroundColor Cyan
Write-Host ""

$cache11 = Join-Path $env:USERPROFILE "AppData\Roaming\Ableton\Live 11\Preferences\VstPlugins"
$cache12 = Join-Path $env:USERPROFILE "AppData\Roaming\Ableton\Live 12\Preferences\VstPlugins"

$found = $false

if (Test-Path $cache11) {
    Write-Host "Found Live 11 cache: $cache11" -ForegroundColor Green
    Get-ChildItem -Path $cache11 -Filter "*CloudWash*.cfg" -ErrorAction SilentlyContinue | ForEach-Object {
        Remove-Item $_.FullName -Force
        Write-Host "  Deleted: $($_.Name)" -ForegroundColor Yellow
    }
    $found = $true
}

if (Test-Path $cache12) {
    Write-Host "Found Live 12 cache: $cache12" -ForegroundColor Green
    Get-ChildItem -Path $cache12 -Filter "*CloudWash*.cfg" -ErrorAction SilentlyContinue | ForEach-Object {
        Remove-Item $_.FullName -Force
        Write-Host "  Deleted: $($_.Name)" -ForegroundColor Yellow
    }
    $found = $true
}

if (-not $found) {
    Write-Host "No Ableton cache folders found" -ForegroundColor Yellow
    Write-Host "Your cache may be in a different location" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== Next Steps ===" -ForegroundColor Cyan
Write-Host "1. Close Ableton Live COMPLETELY" -ForegroundColor White
Write-Host "2. Restart Ableton Live" -ForegroundColor White
Write-Host "3. Go to: Preferences > Plug-ins > Rescan" -ForegroundColor White
Write-Host "4. Load CloudWash and verify:" -ForegroundColor White
Write-Host "   - Preset dropdown shows 20 presets" -ForegroundColor Gray
Write-Host "   - Level meters animate with audio" -ForegroundColor Gray
Write-Host "   - Quality switching doesn't crash" -ForegroundColor Gray
Write-Host ""
