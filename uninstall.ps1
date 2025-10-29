$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    $currentScriptPath = $MyInvocation.MyCommand.Path
    Start-Process -FilePath powershell.exe -ArgumentList "-NoExit `"$currentScriptPath`"" -Verb RunAs
    exit 0
}

try {
    $menuName = "Upload to Catbox"
    $regKeyPath = "HKEY_CLASSES_ROOT\*\shell\$menuName"

    Write-Host "Checking if context menu entry exists..."
    & reg.exe query "$regKeyPath" >$null 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Removing context menu entry..."
        & reg.exe delete "$regKeyPath" /f
        if ($LASTEXITCODE -eq 0) {
            Write-Host -ForegroundColor Green "'$menuName' context menu item successfully removed!"
        } else {
            throw "Failed to remove registry key. Exit code: $LASTEXITCODE"
        }
    } else {
        Write-Host -ForegroundColor Yellow "The context menu item was not found or already removed."
    }
} catch {
    Write-Error "An error occurred while removing the context menu item: $_"
    exit 1
}
