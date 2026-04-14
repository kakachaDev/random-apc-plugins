$ErrorActionPreference = "Stop"

# Initialize and update submodules
Write-Host "Initializing and updating Git submodules..." -ForegroundColor Cyan
git submodule update --init --recursive

Write-Host "Setup Complete." -ForegroundColor Green
