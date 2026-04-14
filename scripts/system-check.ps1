<#
.SYNOPSIS
    Automated dependency validation for APC (audio-plugin-coder)
#>

param(
    [string]$Command,
    [string]$JucePath
)

function Write-JsonOutput { param([string]$Json) Write-Output $Json }

function Test-VersionGreaterOrEqual {
    param([string]$Version1, [string]$Version2)
    try { return [version]$Version1 -ge [version]$Version2 } catch { return $false }
}

function Invoke-DetectPlatform {
    $platform = "windows"
    $ver = (Get-CimInstance Win32_OperatingSystem).Version
    Write-JsonOutput -Json ("{`"platform`":`"$platform`",`"version`":`"$ver`"}")
}

function Invoke-CheckPython {
    $min = "3.8"
    try {
        $out = py --version 2>&1
        if (-not $out) { $out = python --version 2>&1 }
        
        if ($out -match "Microsoft Store") {
            Write-JsonOutput -Json ("{`"found`":false,`"error`":`"Microsoft Store Redirect`"}")
            return
        }
        
        $ver = if ($out -match '(\d+\.\d+\.\d+)') { $matches[1] } else { "0.0.0" }
        
        $ok = Test-VersionGreaterOrEqual $ver $min
        Write-JsonOutput -Json ("{`"found`":true,`"version`":`"$ver`",`"ok`":$($ok.ToString().ToLower())}")
    } catch {
        Write-JsonOutput -Json ("{`"found`":false}")
    }
}

function Invoke-CheckVisualStudio {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $ver = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion
        if ($ver) {
            Write-JsonOutput -Json ("{`"found`":true,`"version`":`"$ver`",`"ok`":true}")
            return
        }
    }
    Write-JsonOutput -Json ("{`"found`":false}")
}

function Invoke-CheckCMake {
    try {
        $out = cmake --version 2>&1
        $ver = if ($out -match 'version (\d+\.\d+\.\d+)') { $matches[1] } else { "0.0.0" }
        $ok = Test-VersionGreaterOrEqual $ver "3.22"
        Write-JsonOutput -Json ("{`"found`":true,`"version`":`"$ver`",`"ok`":$($ok.ToString().ToLower())}")
    } catch {
        Write-JsonOutput -Json ("{`"found`":false}")
    }
}

function Invoke-CheckJUCE {
    # FIX: Updated to use _tools instead of tools
    $path = ".\_tools\JUCE"
    
    if (Test-Path "$path\modules\juce_core\juce_core.h") {
        Write-JsonOutput -Json ("{`"found`":true,`"path`":`"$path`",`"ok`":true}")
    } else {
        Write-JsonOutput -Json ("{`"found`":false,`"path`":`"$path`"}")
    }
}

function Invoke-CheckPluginval {
    # FIX: Updated to use _tools instead of tools
    $path = ".\_tools\pluginval\pluginval.exe"
    if (Test-Path $path) {
        Write-JsonOutput -Json ("{`"found`":true,`"path`":`"$path`",`"ok`":true}")
    } else {
        Write-JsonOutput -Json ("{`"found`":false}")
    }
}

function Invoke-CheckAll {
    Write-Output "{"
    Write-Output "  `"platform`": $(Invoke-DetectPlatform),"
    Write-Output "  `"python`": $(Invoke-CheckPython),"
    Write-Output "  `"vs2022`": $(Invoke-CheckVisualStudio),"
    Write-Output "  `"cmake`": $(Invoke-CheckCMake),"
    Write-Output "  `"juce`": $(Invoke-CheckJUCE),"
    Write-Output "  `"pluginval`": $(Invoke-CheckPluginval)"
    Write-Output "}"
}

switch ($Command) {
    "--check-all" { Invoke-CheckAll }
    default { Invoke-CheckAll }
}