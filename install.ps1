param(
    [string]$ExePath = "",
    [string]$Icon = ""
)

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    $currentScriptPath = $MyInvocation.MyCommand.Path
    $FullExePath = (Resolve-Path $ExePath).Path
    $argumentList = "-NoExit `"$currentScriptPath`" -ExePath `"$FullExePath`""
    if ($Icon -ne "") {
        $iconpath = (Resolve-Path $Icon).Path
        $argumentList += " -Icon `"$iconpath`""
    }
    Start-Process -FilePath powershell.exe -ArgumentList $argumentList -Verb RunAs
    exit 0
}

if ($ExePath -eq "") {
    Write-Host -ForegroundColor Red "Please provide the path to the CatboxUploader executable as the -ExePath parameter."
    exit 1
}

if (-not (Test-Path $ExePath)) {
    Write-Error "Executable not found at path: $ExePath"
    exit 1
}

try {
    $menuName = "Upload to Catbox"
    $regKeyPath = "HKEY_CLASSES_ROOT\*\shell\$menuName"
    $commandKeyPath = "$regKeyPath\command"
    $FullExePath = (Resolve-Path $ExePath).Path

    Write-Host "Creating registry key..."
    & reg.exe add "$regKeyPath" /ve /d "$menuName" /f
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to create main registry key. Exit code: $LASTEXITCODE"
    }

    Write-Host "Creating command registry key..."
    $commandValueEscaped = "`"`"$FullExePath`"`" `"`"`%1`"`""  # me when escaping:
    & reg.exe add "$commandKeyPath" /ve /d $commandValueEscaped /f
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to create command registry key. Exit code: $LASTEXITCODE"
    }

    if ($Icon -ne "") {
        if (-not (Test-Path $Icon)) {
            Write-Warning "Icon file not found at path: $Icon. Skipping icon registration."
        } else {
            Write-Host "Adding icon..."
            $FullIconPath = (Resolve-Path $Icon).Path
            & reg.exe add "$regKeyPath" /v "Icon" /d "$FullIconPath" /f
            if ($LASTEXITCODE -ne 0) {
                Write-Warning "Failed to add icon. Exit code: $LASTEXITCODE"
            }
        }
    }

    Write-Host -ForegroundColor Green "'$menuName' context menu item successfully installed!"
} catch {
    Write-Error "An error occurred while setting up the context menu item: $_"
    exit 1
}
