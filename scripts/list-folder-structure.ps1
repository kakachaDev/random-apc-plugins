param (
    # Root folder to scan
    [Parameter(Mandatory = $true)]
    [string]$RootPath,

    # How deep to scan (0 = only root, 1 = root + immediate children, etc.)
    [int]$Depth = 1,

    # Include files; if not specified, only folders are listed
    [switch]$IncludeFiles,

    # Optional output TXT file
    [string]$OutputFile
)

# Validate root path
if (-not (Test-Path -Path $RootPath -PathType Container)) {
    Write-Error "The specified RootPath does not exist or is not a folder."
    exit 1
}

# Build Get-ChildItem parameters
$gciParams = @{
    Path        = $RootPath
    Recurse    = $true
    Depth      = $Depth
    ErrorAction = "SilentlyContinue"
}

# If files are NOT included, limit to directories only
if (-not $IncludeFiles) {
    $gciParams["Directory"] = $true
}

# Get clean string output (no formatting artifacts)
$results = Get-ChildItem @gciParams |
           Select-Object -ExpandProperty FullName

# Output to file or console
if ($OutputFile) {
    $results | Out-File -FilePath $OutputFile -Encoding UTF8
    Write-Host "Output written to $OutputFile"
} else {
    $results
}
