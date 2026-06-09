$ErrorActionPreference = "Stop"

$ProjectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$VenvPython = Join-Path $ProjectDir ".venv\Scripts\python.exe"
$App = Join-Path $ProjectDir "CatTerminal.py"

function New-ProjectVenv {
    Write-Host "Creating virtual environment..."
    if (Get-Command py -ErrorAction SilentlyContinue) {
        py -3 -m venv (Join-Path $ProjectDir ".venv")
    } elseif (Get-Command python -ErrorAction SilentlyContinue) {
        python -m venv (Join-Path $ProjectDir ".venv")
    } else {
        throw "Python was not found. Install Python 3.11+ from https://www.python.org/downloads/ and rerun this script."
    }
}

if (Test-Path $VenvPython) {
    & $VenvPython --version *> $null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Existing virtual environment is broken; recreating it..."
        Remove-Item -Recurse -Force (Join-Path $ProjectDir ".venv")
        New-ProjectVenv
    }
} else {
    New-ProjectVenv
}

Write-Host "Installing build dependencies..."
& $VenvPython -m pip install --upgrade pip
& $VenvPython -m pip install -r (Join-Path $ProjectDir "requirements.txt")

Write-Host "Building CatTerminal.exe..."
& $VenvPython -m PyInstaller `
    --noconfirm `
    --clean `
    --onefile `
    --windowed `
    --name CatTerminal `
    --collect-submodules serial `
    $App

Write-Host ""
Write-Host "Done. Send this file to other Windows users:"
Write-Host (Join-Path $ProjectDir "dist\CatTerminal.exe")
