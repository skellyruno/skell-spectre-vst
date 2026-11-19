#Requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$InstallerPath,
    [Parameter(Mandatory)][string]$PackageRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$script:Assertions = 0
$script:ProductName = 'SKELL SPECTRE'
$script:BundleName = 'SKELL SPECTRE.vst3'
$script:BinaryRelative = 'Contents/x86_64-win/SKELL SPECTRE.vst3'
$script:ModuleInfoRelative = 'Contents/Resources/moduleinfo.json'
$script:ClassIds = @(
    'ABCDEF019182FAEB536B417553706374',
    'ABCDEF011234ABCD536B417553706374'
)

$InstallerPath = [IO.Path]::GetFullPath($InstallerPath)
$PackageRoot = [IO.Path]::GetFullPath($PackageRoot)
if (-not (Test-Path -LiteralPath $InstallerPath -PathType Leaf)) { throw "Installer script missing: $InstallerPath" }
if (-not (Test-Path -LiteralPath $PackageRoot -PathType Container)) { throw "Preview package missing: $PackageRoot" }

function Assert-Test {
    param([Parameter(Mandatory)][bool]$Condition, [Parameter(Mandatory)][string]$Label)
    if (-not $Condition) { throw "FAIL: $Label" }
    $script:Assertions++
}

function Assert-Fails {
    param([Parameter(Mandatory)][scriptblock]$Action, [Parameter(Mandatory)][string]$Label)
    $failed = $false
    try { & $Action } catch { $failed = $true }
    Assert-Test -Condition $failed -Label $Label
}

function New-TestPe {
    param([Parameter(Mandatory)][string]$Path)
    $bytes = [byte[]]::new(512)
    $bytes[0] = 0x4d
    $bytes[1] = 0x5a
    [Array]::Copy([BitConverter]::GetBytes([int]0x80), 0, $bytes, 0x3c, 4)
    $bytes[0x80] = 0x50
    $bytes[0x81] = 0x45
    $bytes[0x82] = 0
    $bytes[0x83] = 0
    [Array]::Copy([BitConverter]::GetBytes([UInt16]0x8664), 0, $bytes, 0x84, 2)
    [Array]::Copy([BitConverter]::GetBytes([UInt16]1), 0, $bytes, 0x86, 2)
    [Array]::Copy([BitConverter]::GetBytes([UInt16]0x20b), 0, $bytes, 0x98, 2)
    [IO.File]::WriteAllBytes($Path, $bytes)
}

function New-TestModuleInfo {
    param([string]$Version = '1.8.3')
    return [ordered]@{
        Name = $script:ProductName
        Version = $Version
        'Factory Info' = [ordered]@{ Vendor = 'Skell Audio'; URL = ''; 'E-Mail' = '' }
        Classes = @(
            [ordered]@{
                CID = $script:ClassIds[0]
                Category = 'Audio Module Class'
                Name = $script:ProductName
                Vendor = 'Skell Audio'
                Version = $Version
            },
            [ordered]@{
                CID = $script:ClassIds[1]
                Category = 'Component Controller Class'
                Name = $script:ProductName
                Vendor = 'Skell Audio'
                Version = $Version
            }
        )
    }
}

function Write-TestChecksums {
    param([Parameter(Mandatory)][string]$Root)
    $manifestPath = Join-Path $Root 'SHA256SUMS.txt'
    $lines = [System.Collections.Generic.List[string]]::new()
    foreach ($file in (Get-ChildItem -LiteralPath $Root -File -Recurse -Force |
        Where-Object { -not [string]::Equals($_.FullName, $manifestPath, [StringComparison]::OrdinalIgnoreCase) } |
        Sort-Object -Property FullName)) {
        $relative = [IO.Path]::GetRelativePath($Root, $file.FullName).Replace('\', '/')
        $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        $lines.Add("$hash  $relative")
    }
    [IO.File]::WriteAllLines($manifestPath, $lines, [Text.UTF8Encoding]::new($false))
}

function New-TestPackage {
    param([Parameter(Mandatory)][string]$Root)
    $null = New-Item -ItemType Directory -Path $Root -Force
    $bundle = Join-Path $Root $script:BundleName
    $moduleInfo = Join-Path $bundle $script:ModuleInfoRelative
    $binary = Join-Path $bundle $script:BinaryRelative
    $null = New-Item -ItemType Directory -Path (Split-Path -Parent $moduleInfo) -Force
    $null = New-Item -ItemType Directory -Path (Split-Path -Parent $binary) -Force
    [IO.File]::WriteAllText($moduleInfo, (ConvertTo-Json -InputObject (New-TestModuleInfo) -Depth 10) + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
    New-TestPe -Path $binary

    Copy-Item -LiteralPath $InstallerPath -Destination (Join-Path $Root 'Install-SKELL-SPECTRE-Preview.ps1')
    [IO.File]::WriteAllText((Join-Path $Root 'PREVIEW-ONLY.txt'), 'Private preview only. Not for resale or redistribution.' + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $Root 'WINDOWS-VST3-INSTALL.md'), 'Test fixture install guide.' + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $Root 'THIRD_PARTY_NOTICES.txt'), 'STATUS: INCOMPLETE' + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))

    $binaryHash = (Get-FileHash -LiteralPath $binary -Algorithm SHA256).Hash.ToLowerInvariant()
    $moduleHash = (Get-FileHash -LiteralPath $moduleInfo -Algorithm SHA256).Hash.ToLowerInvariant()
    $manifest = [ordered]@{
        product = $script:ProductName
        vendor = 'Skell Audio'
        version = '1.8.3'
        package_type = 'windows-x64-vst3-preview'
        release_status = 'PREVIEW_ONLY'
        source_commit = ('a' * 40)
        ci_checkout_commit = ('b' * 40)
        ref_name = 'test'
        workflow_run_id = '12345'
        platform = 'Windows x64'
        binary_path = 'SKELL SPECTRE.vst3/Contents/x86_64-win/SKELL SPECTRE.vst3'
        binary_size_bytes = (Get-Item -LiteralPath $binary).Length
        binary_sha256 = $binaryHash
        moduleinfo_path = 'SKELL SPECTRE.vst3/Contents/Resources/moduleinfo.json'
        moduleinfo_original_sha256 = $moduleHash
        moduleinfo_normalized_sha256 = $moduleHash
        architecture = [ordered]@{ machine = 'AMD64'; machine_code = '0x8664'; optional_header = 'PE32+' }
        pluginval = [ordered]@{ version = '1.0.4'; strictness_level = 5; result = 'passed' }
        commercial_release_ready = $false
        commercial_release_blockers = @('vendor URL', 'support email', 'license/EULA and third-party audit')
        runtime_acceptance = 'UNVERIFIED: FL Studio host acceptance is separate'
        audio_acceptance = 'UNVERIFIED: real audio A/B and hardware checks are separate'
        created_utc = (Get-Date).ToUniversalTime().ToString('o')
    }
    [IO.File]::WriteAllText((Join-Path $Root 'BUILD-MANIFEST.json'), (ConvertTo-Json -InputObject $manifest -Depth 12) + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
    Write-TestChecksums -Root $Root
}

function Get-TestVersion {
    param([Parameter(Mandatory)][string]$BundlePath)
    $path = Join-Path $BundlePath $script:ModuleInfoRelative
    $document = [IO.File]::ReadAllText($path) | ConvertFrom-Json -AsHashtable -ErrorAction Stop
    return [string]$document.Version
}

function Set-TestVersion {
    param([Parameter(Mandatory)][string]$BundlePath, [Parameter(Mandatory)][string]$Version)
    $path = Join-Path $BundlePath $script:ModuleInfoRelative
    $document = [IO.File]::ReadAllText($path) | ConvertFrom-Json -AsHashtable -ErrorAction Stop
    $document.Version = $Version
    foreach ($class in $document.Classes) { $class.Version = $Version }
    [IO.File]::WriteAllText($path, (ConvertTo-Json -InputObject $document -Depth 10) + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
}

function Invoke-TestInstaller {
    param(
        [string]$Package,
        [string]$Destination,
        [string]$BackupRoot,
        [string]$RollbackBackup,
        [switch]$WhatIf
    )
    $invokeParams = @{ Destination = $Destination; BackupRoot = $BackupRoot; Confirm = $false }
    if (-not [string]::IsNullOrWhiteSpace($Package)) { $invokeParams.PackageRoot = $Package }
    if (-not [string]::IsNullOrWhiteSpace($RollbackBackup)) { $invokeParams.RollbackBackup = $RollbackBackup }
    if ($WhatIf) { $invokeParams.WhatIf = $true }
    return (& $InstallerPath @invokeParams 2>&1 | Out-String)
}

$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('SkellSpectreInstallerTests-' + [guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $testRoot -Force
try {
    $actualWhatIfDestination = Join-Path $testRoot 'Actual-WhatIf-VST3'
    $actualWhatIfBackup = Join-Path $testRoot 'Actual-WhatIf-Backups'
    $null = Invoke-TestInstaller -Package $PackageRoot -Destination $actualWhatIfDestination -BackupRoot $actualWhatIfBackup -WhatIf
    Assert-Test -Condition (-not (Test-Path -LiteralPath $actualWhatIfDestination)) -Label 'actual package WhatIf does not create the install directory'
    Assert-Test -Condition (-not (Test-Path -LiteralPath $actualWhatIfBackup)) -Label 'actual package WhatIf does not create the backup directory'

    $package = Join-Path $testRoot 'fixture-package'
    New-TestPackage -Root $package
    $destination = Join-Path $testRoot 'VST3'
    $backupRoot = Join-Path $testRoot 'Backups'
    $whatIfDestination = Join-Path $testRoot 'Fixture-WhatIf-VST3'
    $whatIfBackup = Join-Path $testRoot 'Fixture-WhatIf-Backups'

    $null = Invoke-TestInstaller -Package $package -Destination $whatIfDestination -BackupRoot $whatIfBackup -WhatIf
    Assert-Test -Condition (-not (Test-Path -LiteralPath $whatIfDestination)) -Label 'fixture WhatIf leaves install directory absent'
    Assert-Test -Condition (-not (Test-Path -LiteralPath $whatIfBackup)) -Label 'fixture WhatIf leaves backup directory absent'

    $null = Invoke-TestInstaller -Package $package -Destination $destination -BackupRoot $backupRoot
    $target = Join-Path $destination $script:BundleName
    Assert-Test -Condition ((Get-TestVersion -BundlePath $target) -eq '1.8.3') -Label 'fresh install places the expected VST3 version'
    $null = Invoke-TestInstaller -Package $package -Destination $destination -BackupRoot $backupRoot
    $backupChildren = if (Test-Path -LiteralPath $backupRoot) { @(Get-ChildItem -LiteralPath $backupRoot -Directory) } else { @() }
    Assert-Test -Condition ($backupChildren.Count -eq 0) -Label 'same-hash reinstall is idempotent and creates no backup'

    Set-TestVersion -BundlePath $target -Version '1.8.2'
    $updateOutput = Invoke-TestInstaller -Package $package -Destination $destination -BackupRoot $backupRoot
    Assert-Test -Condition ((Get-TestVersion -BundlePath $target) -eq '1.8.3') -Label 'update installs the new VST3 version'
    $backupMatch = [regex]::Match($updateOutput, '(?m)^BACKUP_PATH=(.+)$')
    Assert-Test -Condition $backupMatch.Success -Label 'update reports its exact backup path'
    $backupPath = $backupMatch.Groups[1].Value.Trim()
    $previousBundle = Join-Path $backupPath $script:BundleName
    Assert-Test -Condition ((Get-TestVersion -BundlePath $previousBundle) -eq '1.8.2') -Label 'update backup preserves the previous version'

    $installedBinary = Join-Path $target $script:BinaryRelative
    $bytes = [IO.File]::ReadAllBytes($installedBinary)
    $bytes[300] = $bytes[300] -bxor 1
    [IO.File]::WriteAllBytes($installedBinary, $bytes)
    $tamperedHash = (Get-FileHash -LiteralPath $installedBinary -Algorithm SHA256).Hash
    Assert-Fails -Action { Invoke-TestInstaller -Destination $destination -BackupRoot $backupRoot -RollbackBackup $backupPath } -Label 'modified installed VST3 blocks rollback'
    Assert-Test -Condition ((Get-FileHash -LiteralPath $installedBinary -Algorithm SHA256).Hash -eq $tamperedHash) -Label 'refused rollback leaves modified binary untouched'

    Remove-Item -LiteralPath $target -Recurse -Force
    Copy-Item -LiteralPath (Join-Path $package $script:BundleName) -Destination $destination -Recurse
    $null = Invoke-TestInstaller -Destination $destination -BackupRoot $backupRoot -RollbackBackup $backupPath
    Assert-Test -Condition ((Get-TestVersion -BundlePath $target) -eq '1.8.2') -Label 'rollback restores the verified previous version'
    $forwardBundle = Join-Path $backupPath 'forward-current'
    Assert-Test -Condition ((Get-TestVersion -BundlePath $forwardBundle) -eq '1.8.3') -Label 'rollback preserves the newer version for recovery'

    $tamperedPackage = Join-Path $testRoot 'tampered-package'
    Copy-Item -LiteralPath $package -Destination $tamperedPackage -Recurse
    $tamperedBinary = Join-Path (Join-Path $tamperedPackage $script:BundleName) $script:BinaryRelative
    $tamperedBytes = [IO.File]::ReadAllBytes($tamperedBinary)
    $tamperedBytes[301] = $tamperedBytes[301] -bxor 1
    [IO.File]::WriteAllBytes($tamperedBinary, $tamperedBytes)
    $tamperedDestination = Join-Path $testRoot 'Tampered-VST3'
    Assert-Fails -Action { Invoke-TestInstaller -Package $tamperedPackage -Destination $tamperedDestination -BackupRoot (Join-Path $testRoot 'Tampered-Backups') } -Label 'tampered package checksum is rejected'
    Assert-Test -Condition (-not (Test-Path -LiteralPath $tamperedDestination)) -Label 'tampered package does not create an install directory'

    $duplicatePackage = Join-Path $testRoot 'duplicate-package'
    Copy-Item -LiteralPath $package -Destination $duplicatePackage -Recurse
    Copy-Item -LiteralPath $tamperedBinary -Destination (Join-Path $duplicatePackage 'rogue.vst3')
    Write-TestChecksums -Root $duplicatePackage
    $duplicateDestination = Join-Path $testRoot 'Duplicate-VST3'
    Assert-Fails -Action { Invoke-TestInstaller -Package $duplicatePackage -Destination $duplicateDestination -BackupRoot (Join-Path $testRoot 'Duplicate-Backups') } -Label 'extra VST3 module outside the bundle is rejected'
    Assert-Test -Condition (-not (Test-Path -LiteralPath $duplicateDestination)) -Label 'duplicate-module package does not create an install directory'

    $executablePackage = Join-Path $testRoot 'executable-package'
    Copy-Item -LiteralPath $package -Destination $executablePackage -Recurse
    [IO.File]::WriteAllBytes((Join-Path $executablePackage 'rogue.exe'), [byte[]](1, 2, 3))
    Write-TestChecksums -Root $executablePackage
    $executableDestination = Join-Path $testRoot 'Executable-VST3'
    Assert-Fails -Action { Invoke-TestInstaller -Package $executablePackage -Destination $executableDestination -BackupRoot (Join-Path $testRoot 'Executable-Backups') } -Label 'unexpected executable in package is rejected'
    Assert-Test -Condition (-not (Test-Path -LiteralPath $executableDestination)) -Label 'package with an executable does not create an install directory'

    $invalidDestination = Join-Path $testRoot 'Invalid-VST3'
    $null = New-Item -ItemType Directory -Path $invalidDestination -Force
    $invalidTarget = Join-Path $invalidDestination $script:BundleName
    [IO.File]::WriteAllText($invalidTarget, 'preserve', [Text.UTF8Encoding]::new($false))
    Assert-Fails -Action { Invoke-TestInstaller -Package $package -Destination $invalidDestination -BackupRoot (Join-Path $testRoot 'Invalid-Backups') } -Label 'non-directory existing target is rejected'
    Assert-Test -Condition ([IO.File]::ReadAllText($invalidTarget) -eq 'preserve') -Label 'invalid existing target remains untouched'

    Write-Host "PASS: $script:Assertions SKELL SPECTRE installer/update/rollback assertions"
} finally {
    if (Test-Path -LiteralPath $testRoot) { Remove-Item -LiteralPath $testRoot -Recurse -Force }
}
