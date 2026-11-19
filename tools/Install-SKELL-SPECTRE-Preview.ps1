#Requires -Version 7.0
[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'High')]
param(
    [string]$PackageRoot,
    [string]$Destination,
    [string]$BackupRoot,
    [string]$RollbackBackup
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:ProductName = 'SKELL SPECTRE'
$script:VendorName = 'Skell Audio'
$script:BundleName = 'SKELL SPECTRE.vst3'
$script:BinaryRelative = 'Contents/x86_64-win/SKELL SPECTRE.vst3'
$script:ModuleInfoRelative = 'Contents/Resources/moduleinfo.json'
$script:ExpectedClassIds = @(
    'abcdef019182faeb536b417553706374',
    'abcdef011234abcd536b417553706374'
)

function Get-FullPath {
    param([Parameter(Mandatory)][string]$Path)
    return [IO.Path]::GetFullPath($Path)
}

function Test-PathWithin {
    param([Parameter(Mandatory)][string]$Parent, [Parameter(Mandatory)][string]$Child)
    $parentFull = Get-FullPath $Parent
    $childFull = Get-FullPath $Child
    if ([string]::Equals($parentFull, $childFull, [StringComparison]::OrdinalIgnoreCase)) { return $true }
    $separator = [IO.Path]::DirectorySeparatorChar
    $prefix = if ($parentFull.EndsWith([string]$separator)) { $parentFull } else { $parentFull + $separator }
    return $childFull.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)
}

function Test-PathsOverlap {
    param([Parameter(Mandatory)][string]$Left, [Parameter(Mandatory)][string]$Right)
    return (Test-PathWithin -Parent $Left -Child $Right) -or (Test-PathWithin -Parent $Right -Child $Left)
}

function Assert-NonRootPath {
    param([Parameter(Mandatory)][string]$Path)
    $full = Get-FullPath $Path
    $root = [IO.Path]::GetPathRoot($full)
    $trimmedFull = $full.TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar)
    $trimmedRoot = $root.TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar)
    if ([string]::Equals($trimmedFull, $trimmedRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing a filesystem root: $full"
    }
}

function Assert-NoReparseAncestors {
    param([Parameter(Mandatory)][string]$Path)
    $cursor = Get-FullPath $Path
    while (-not [string]::IsNullOrWhiteSpace($cursor)) {
        if (Test-Path -LiteralPath $cursor) {
            $item = Get-Item -LiteralPath $cursor -Force
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Refusing a path beneath a reparse point: $cursor"
            }
        }
        $parent = Split-Path -Path $cursor -Parent
        if ([string]::IsNullOrWhiteSpace($parent) -or
            [string]::Equals($parent, $cursor, [StringComparison]::OrdinalIgnoreCase)) { break }
        $cursor = $parent
    }
}

function Assert-NoReparsePoints {
    param([Parameter(Mandatory)][string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) { return }
    $item = Get-Item -LiteralPath $Path -Force
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Refusing a reparse point: $Path"
    }
    if ($item.PSIsContainer) {
        foreach ($child in (Get-ChildItem -LiteralPath $Path -Force)) {
            if (($child.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Refusing a tree containing a reparse point: $($child.FullName)"
            }
            if ($child.PSIsContainer) { Assert-NoReparsePoints -Path $child.FullName }
        }
    }
}

function Ensure-SafeDirectory {
    param([Parameter(Mandatory)][string]$Path)
    Assert-NonRootPath -Path $Path
    Assert-NoReparseAncestors -Path $Path
    if (Test-Path -LiteralPath $Path) {
        if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
            throw "Expected a directory: $Path"
        }
    } else {
        $null = New-Item -ItemType Directory -Path $Path -Force
    }
    Assert-NoReparsePoints -Path $Path
}

function Assert-SafeInstallPaths {
    param(
        [Parameter(Mandatory)][string]$PackageRoot,
        [Parameter(Mandatory)][string]$Destination,
        [Parameter(Mandatory)][string]$BackupRoot
    )
    foreach ($path in @($PackageRoot, $Destination, $BackupRoot)) { Assert-NonRootPath -Path $path }
    if (Test-PathsOverlap -Left $PackageRoot -Right $Destination) { throw 'Package and VST3 destination must not overlap' }
    if (Test-PathsOverlap -Left $PackageRoot -Right $BackupRoot) { throw 'Package and backup root must not overlap' }
    if (Test-PathsOverlap -Left $Destination -Right $BackupRoot) { throw 'VST3 destination and backup root must not overlap' }
    Assert-NoReparsePoints -Path $PackageRoot
    Assert-NoReparseAncestors -Path $Destination
    Assert-NoReparseAncestors -Path $BackupRoot
    if (Test-Path -LiteralPath $Destination) { Assert-NoReparsePoints -Path $Destination }
    if (Test-Path -LiteralPath $BackupRoot) { Assert-NoReparsePoints -Path $BackupRoot }
}

function Get-TreeSha256 {
    param([Parameter(Mandatory)][string]$Path)
    $root = Get-FullPath $Path
    if (-not (Test-Path -LiteralPath $root -PathType Container)) { throw "Bundle directory missing: $root" }
    Assert-NoReparsePoints -Path $root
    $files = @(Get-ChildItem -LiteralPath $root -File -Recurse -Force | Sort-Object -Property FullName)
    if ($files.Count -eq 0) { throw "Bundle contains no files: $root" }
    $lines = [System.Collections.Generic.List[string]]::new()
    foreach ($file in $files) {
        $relative = [IO.Path]::GetRelativePath($root, $file.FullName).Replace('\', '/')
        $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        $lines.Add("$relative  $hash")
    }
    $payload = [Text.Encoding]::UTF8.GetBytes(($lines -join [Environment]::NewLine) + [Environment]::NewLine)
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString($sha.ComputeHash($payload))).Replace('-', '').ToLowerInvariant()
    } finally {
        $sha.Dispose()
    }
}

function Test-PeX64 {
    param([Parameter(Mandatory)][string]$Path)
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 512 -or $bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a) {
        throw "VST3 module is not a valid Windows PE file: $Path"
    }
    $offset = [BitConverter]::ToInt32($bytes, 0x3c)
    if ($offset -lt 64 -or $offset + 26 -gt $bytes.Length) { throw "Invalid PE header offset: $Path" }
    if ($bytes[$offset] -ne 0x50 -or $bytes[$offset + 1] -ne 0x45 -or
        $bytes[$offset + 2] -ne 0 -or $bytes[$offset + 3] -ne 0) {
        throw "PE signature missing: $Path"
    }
    $machine = [BitConverter]::ToUInt16($bytes, $offset + 4)
    $sections = [BitConverter]::ToUInt16($bytes, $offset + 6)
    $magic = [BitConverter]::ToUInt16($bytes, $offset + 24)
    if ($machine -ne 0x8664 -or $magic -ne 0x20b -or $sections -eq 0) {
        throw "VST3 module must be AMD64 PE32+ with at least one section: $Path"
    }
}

function Read-ModuleIdentity {
    param([Parameter(Mandatory)][string]$BundlePath)
    Assert-NoReparsePoints -Path $BundlePath
    $moduleInfoPath = Join-Path $BundlePath $script:ModuleInfoRelative
    $modulePath = Join-Path $BundlePath $script:BinaryRelative
    if (-not (Test-Path -LiteralPath $moduleInfoPath -PathType Leaf)) { throw 'VST3 moduleinfo.json is missing' }
    if (-not (Test-Path -LiteralPath $modulePath -PathType Leaf)) { throw 'Expected Windows x64 VST3 module is missing' }

    $modules = @(Get-ChildItem -LiteralPath $BundlePath -File -Recurse -Filter '*.vst3')
    if ($modules.Count -ne 1 -or
        -not [string]::Equals((Get-FullPath $modules[0].FullName), (Get-FullPath $modulePath), [StringComparison]::OrdinalIgnoreCase)) {
        throw 'VST3 bundle must contain exactly one module at the expected Windows x64 path'
    }

    $document = [IO.File]::ReadAllText($moduleInfoPath) | ConvertFrom-Json -AsHashtable -ErrorAction Stop
    $version = [string]$document.Version
    if ($document.Name -ne $script:ProductName -or $version -notmatch '^\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$') {
        throw 'VST3 product identity or semantic version is invalid'
    }
    if ($document['Factory Info'].Vendor -ne $script:VendorName) { throw 'VST3 vendor identity is invalid' }

    $classes = @($document.Classes)
    if ($classes.Count -ne 2) { throw 'VST3 must contain exactly one processor and one controller class' }
    $ids = [System.Collections.Generic.List[string]]::new()
    $categories = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($class in $classes) {
        if ($class.Name -ne $script:ProductName -or $class.Vendor -ne $script:VendorName -or
            [string]$class.Version -ne $version) {
            throw 'VST3 class product, vendor or version mismatch'
        }
        $cid = ([string]$class.CID).ToLowerInvariant()
        if ($cid -notmatch '^[0-9a-f]{32}$') { throw 'VST3 class CID is invalid' }
        $ids.Add($cid)
        $categories.Add([string]$class.Category) | Out-Null
    }
    if (@($ids | Select-Object -Unique).Count -ne 2 -or $categories.Count -ne 2) {
        throw 'VST3 class IDs and categories must be unique'
    }
    if (-not $categories.Contains('Audio Module Class') -or
        -not $categories.Contains('Component Controller Class')) {
        throw 'VST3 processor/controller categories are invalid'
    }
    $actualIds = @($ids | Sort-Object)
    $expectedIds = @($script:ExpectedClassIds | Sort-Object)
    if (($actualIds -join '|') -ne ($expectedIds -join '|')) {
        throw 'VST3 class IDs differ from the preserved SKELL SPECTRE host identity'
    }

    Test-PeX64 -Path $modulePath
    return [pscustomobject]@{
        Version = $version
        ClassIds = @($ids)
        BinaryPath = 'SKELL SPECTRE.vst3/Contents/x86_64-win/SKELL SPECTRE.vst3'
        BinarySha256 = (Get-FileHash -LiteralPath $modulePath -Algorithm SHA256).Hash.ToLowerInvariant()
        BinarySize = (Get-Item -LiteralPath $modulePath).Length
        ModuleInfoPath = 'SKELL SPECTRE.vst3/Contents/Resources/moduleinfo.json'
        ModuleInfoSha256 = (Get-FileHash -LiteralPath $moduleInfoPath -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}

function Test-ChecksumManifest {
    param([Parameter(Mandatory)][string]$PackageRoot)
    $root = Get-FullPath $PackageRoot
    $manifestPath = Join-Path $root 'SHA256SUMS.txt'
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw 'SHA256SUMS.txt is missing' }

    $expected = [System.Collections.Generic.Dictionary[string, string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($line in [IO.File]::ReadAllLines($manifestPath)) {
        if ($line -notmatch '^([0-9a-fA-F]{64})  (.+)$') { throw "Invalid checksum line: $line" }
        $relative = $Matches[2].Replace('\', '/')
        $parts = $relative.Split('/')
        if ($relative.StartsWith('/') -or $relative.Contains(':') -or
            $parts.Count -eq 0 -or $parts -contains '' -or $parts -contains '.' -or $parts -contains '..') {
            throw "Unsafe checksum path: $relative"
        }
        if ($expected.ContainsKey($relative)) { throw "Duplicate or case-colliding checksum path: $relative" }
        $native = $relative.Replace('/', [string][IO.Path]::DirectorySeparatorChar)
        $full = Get-FullPath (Join-Path $root $native)
        if (-not (Test-PathWithin -Parent $root -Child $full)) { throw "Checksum path escapes package root: $relative" }
        if (-not (Test-Path -LiteralPath $full -PathType Leaf)) { throw "Checksummed file is missing: $relative" }
        $expected.Add($relative, $Matches[1].ToLowerInvariant())
    }

    $actualFiles = @(Get-ChildItem -LiteralPath $root -File -Recurse -Force |
        Where-Object { -not [string]::Equals((Get-FullPath $_.FullName), (Get-FullPath $manifestPath), [StringComparison]::OrdinalIgnoreCase) })
    if ($actualFiles.Count -ne $expected.Count) {
        throw "Checksum inventory mismatch: expected $($expected.Count) files, found $($actualFiles.Count)"
    }
    foreach ($file in $actualFiles) {
        $relative = [IO.Path]::GetRelativePath($root, $file.FullName).Replace('\', '/')
        if (-not $expected.ContainsKey($relative)) { throw "Unlisted package file: $relative" }
        $actualHash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($actualHash -ne $expected[$relative]) { throw "Checksum mismatch: $relative" }
    }
}

function Test-PreviewPackage {
    param([Parameter(Mandatory)][string]$PackageRoot)
    $root = Get-FullPath $PackageRoot
    if (-not (Test-Path -LiteralPath $root -PathType Container)) { throw "Package root is missing: $root" }
    Assert-NoReparsePoints -Path $root

    foreach ($required in @(
        'BUILD-MANIFEST.json',
        'SHA256SUMS.txt',
        'PREVIEW-ONLY.txt',
        'Install-SKELL-SPECTRE-Preview.ps1',
        'WINDOWS-VST3-INSTALL.md',
        'THIRD_PARTY_NOTICES.txt'
    )) {
        if (-not (Test-Path -LiteralPath (Join-Path $root $required) -PathType Leaf)) {
            throw "Required private-preview package file is missing: $required"
        }
    }

    $bundles = @(Get-ChildItem -LiteralPath $root -Directory -Recurse -Filter '*.vst3')
    $expectedBundle = Join-Path $root $script:BundleName
    if ($bundles.Count -ne 1 -or
        -not [string]::Equals((Get-FullPath $bundles[0].FullName), (Get-FullPath $expectedBundle), [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Package must contain exactly one flat SKELL SPECTRE.vst3 bundle'
    }
    $bundle = $bundles[0].FullName
    $allModules = @(Get-ChildItem -LiteralPath $root -File -Recurse -Filter '*.vst3')
    $expectedModule = Join-Path $bundle $script:BinaryRelative
    if ($allModules.Count -ne 1 -or
        -not [string]::Equals((Get-FullPath $allModules[0].FullName), (Get-FullPath $expectedModule), [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Preview package must contain exactly one VST3 module at the expected bundle path'
    }
    $forbidden = @(Get-ChildItem -LiteralPath $root -File -Recurse -Force |
        Where-Object { $_.Extension.ToLowerInvariant() -in @('.exe', '.zip', '.app', '.component') })
    if ($forbidden.Count -ne 0) { throw 'Preview package contains a forbidden executable or non-VST3 format' }

    $identity = Read-ModuleIdentity -BundlePath $bundle
    $manifest = [IO.File]::ReadAllText((Join-Path $root 'BUILD-MANIFEST.json')) |
        ConvertFrom-Json -AsHashtable -ErrorAction Stop
    if ($manifest.product -ne $script:ProductName -or $manifest.vendor -ne $script:VendorName -or
        [string]$manifest.version -ne $identity.Version -or
        $manifest.package_type -ne 'windows-x64-vst3-preview' -or
        $manifest.release_status -ne 'PREVIEW_ONLY' -or $manifest.platform -ne 'Windows x64') {
        throw 'BUILD-MANIFEST.json product, version, platform or preview status mismatch'
    }
    if ([string]$manifest.source_commit -notmatch '^[0-9a-fA-F]{40}$' -or
        [string]$manifest.ci_checkout_commit -notmatch '^[0-9a-fA-F]{40}$' -or
        [string]$manifest.workflow_run_id -notmatch '^\d+$') {
        throw 'BUILD-MANIFEST.json is missing exact source/CI provenance'
    }
    if ([long]$manifest.binary_size_bytes -ne [long]$identity.BinarySize -or
        $manifest.binary_path -ne $identity.BinaryPath -or
        $manifest.binary_sha256 -ne $identity.BinarySha256 -or
        $manifest.moduleinfo_path -ne $identity.ModuleInfoPath -or
        $manifest.moduleinfo_normalized_sha256 -ne $identity.ModuleInfoSha256) {
        throw 'BUILD-MANIFEST.json binary or moduleinfo identity/hash mismatch'
    }
    if ($manifest.architecture.machine -ne 'AMD64' -or
        $manifest.architecture.machine_code -ne '0x8664' -or
        $manifest.architecture.optional_header -ne 'PE32+') {
        throw 'BUILD-MANIFEST.json does not confirm AMD64 PE32+ architecture'
    }
    if ($manifest.pluginval.result -ne 'passed' -or
        [int]$manifest.pluginval.strictness_level -ne 5 -or
        $manifest.pluginval.version -ne '1.0.4') {
        throw 'Exact staged VST3 must have a recorded pluginval 1.0.4 strictness-5 pass'
    }
    if ($manifest.commercial_release_ready -eq $true) {
        throw 'Commercial-ready metadata is not accepted by this private-preview installer'
    }
    if ([string]$manifest.runtime_acceptance -notlike 'UNVERIFIED:*' -or
        [string]$manifest.audio_acceptance -notlike 'UNVERIFIED:*' -or
        @($manifest.commercial_release_blockers).Count -eq 0) {
        throw 'Private-preview manifest must preserve explicit host/audio and commercial blockers'
    }
    if ([string]::IsNullOrWhiteSpace([string]$manifest.ref_name)) {
        throw 'BUILD-MANIFEST.json is missing its source ref name'
    }

    Test-ChecksumManifest -PackageRoot $root
    return [pscustomobject]@{
        PackageRoot = $root
        BundlePath = $bundle
        Version = $identity.Version
        TreeSha256 = Get-TreeSha256 -Path $bundle
    }
}

function Test-InstalledBundle {
    param([Parameter(Mandatory)][string]$BundlePath)
    $identity = Read-ModuleIdentity -BundlePath $BundlePath
    return [pscustomobject]@{
        Version = $identity.Version
        TreeSha256 = Get-TreeSha256 -Path $BundlePath
    }
}

function Write-JsonFile {
    param([Parameter(Mandatory)][string]$Path, [Parameter(Mandatory)][object]$Value)
    $json = ConvertTo-Json -InputObject $Value -Depth 12
    [IO.File]::WriteAllText($Path, $json + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
}

function Get-RollbackPlan {
    param(
        [Parameter(Mandatory)][string]$BackupRoot,
        [Parameter(Mandatory)][string]$RollbackBackup,
        [Parameter(Mandatory)][string]$Destination
    )
    $backupRootFull = Get-FullPath $BackupRoot
    $backupFull = Get-FullPath $RollbackBackup
    Assert-NonRootPath -Path $backupFull
    if ([string]::Equals($backupRootFull, $backupFull, [StringComparison]::OrdinalIgnoreCase) -or
        -not (Test-PathWithin -Parent $backupRootFull -Child $backupFull)) {
        throw 'Rollback backup must be a specific child directory of the configured backup root'
    }
    if (-not (Test-Path -LiteralPath $backupFull -PathType Container)) { throw 'Rollback backup directory is missing' }
    Assert-NoReparsePoints -Path $backupFull

    $statePath = Join-Path $backupFull 'backup-manifest.json'
    if (-not (Test-Path -LiteralPath $statePath -PathType Leaf)) { throw 'Rollback backup manifest is missing' }
    $state = [IO.File]::ReadAllText($statePath) | ConvertFrom-Json -AsHashtable -ErrorAction Stop
    if ($state.schema -ne 'skellspectre-backup/1' -or $state.product -ne $script:ProductName) {
        throw 'Rollback backup manifest has an unsupported schema or product identity'
    }

    $recordedDestination = Get-FullPath ([string]$state.destination)
    if (-not [string]::Equals((Get-FullPath $Destination), $recordedDestination, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Requested VST3 destination does not match the recorded installation path'
    }
    Assert-NoReparseAncestors -Path $recordedDestination
    if (-not (Test-Path -LiteralPath $recordedDestination -PathType Container)) { throw 'Recorded VST3 destination is missing' }
    Assert-NoReparsePoints -Path $recordedDestination

    $target = Join-Path $recordedDestination $script:BundleName
    if (-not (Test-Path -LiteralPath $target -PathType Container)) { throw 'Current installed VST3 bundle is missing' }
    $current = Test-InstalledBundle -BundlePath $target
    if ($current.Version -ne [string]$state.installed_version -or
        $current.TreeSha256 -ne [string]$state.installed_sha256) {
        throw 'Installed VST3 changed after this update; refusing to overwrite user or host changes'
    }

    if ($state.previous_bundle -ne $script:BundleName) { throw 'Rollback manifest references an unexpected bundle name' }
    $previousBundle = Join-Path $backupFull $script:BundleName
    if (-not (Test-Path -LiteralPath $previousBundle -PathType Container)) { throw 'Previous VST3 backup is missing' }
    $previous = Test-InstalledBundle -BundlePath $previousBundle
    if ($previous.Version -ne [string]$state.previous_version -or
        $previous.TreeSha256 -ne [string]$state.previous_sha256) {
        throw 'Previous VST3 backup hash or version does not match its manifest'
    }

    return [pscustomobject]@{
        BackupPath = $backupFull
        Destination = $recordedDestination
        Target = $target
        PreviousBundle = $previousBundle
        PreviousVersion = $previous.Version
        PreviousSha256 = $previous.TreeSha256
        CurrentVersion = $current.Version
        CurrentSha256 = $current.TreeSha256
    }
}

function Invoke-InstallInternal {
    param(
        [Parameter(Mandatory)][string]$PackageRoot,
        [Parameter(Mandatory)][string]$Destination,
        [Parameter(Mandatory)][string]$BackupRoot
    )
    $package = Test-PreviewPackage -PackageRoot $PackageRoot
    $destinationFull = Get-FullPath $Destination
    $backupFull = Get-FullPath $BackupRoot
    Assert-SafeInstallPaths -PackageRoot $package.PackageRoot -Destination $destinationFull -BackupRoot $backupFull
    Ensure-SafeDirectory -Path $destinationFull
    Ensure-SafeDirectory -Path $backupFull

    $target = Join-Path $destinationFull $script:BundleName
    $id = [guid]::NewGuid().ToString('N')
    $stageRoot = Join-Path $destinationFull ".skell-spectre-stage-$id"
    Ensure-SafeDirectory -Path $stageRoot
    $oldMove = Join-Path $stageRoot 'previous-live'
    $backupPath = ''

    try {
        Copy-Item -LiteralPath $package.BundlePath -Destination $stageRoot -Recurse
        $stagedBundle = Join-Path $stageRoot $script:BundleName
        $staged = Test-InstalledBundle -BundlePath $stagedBundle
        if ($staged.TreeSha256 -ne $package.TreeSha256) { throw 'Staged VST3 hash does not match the verified package' }

        if (Test-Path -LiteralPath $target) {
            if (-not (Test-Path -LiteralPath $target -PathType Container)) {
                throw 'A non-directory item already occupies the SKELL SPECTRE VST3 destination'
            }
            Assert-NoReparsePoints -Path $target
            $current = Test-InstalledBundle -BundlePath $target
            if ($current.TreeSha256 -eq $package.TreeSha256) {
                Write-Output "Already installed: SKELL SPECTRE $($package.Version)"
                return
            }

            $backupName = (Get-Date -Format 'yyyyMMdd-HHmmssfff') + '-' + $id
            $backupPath = Join-Path $backupFull $backupName
            Ensure-SafeDirectory -Path $backupPath
            Copy-Item -LiteralPath $target -Destination $backupPath -Recurse
            $backupBundle = Join-Path $backupPath $script:BundleName
            $backupCopy = Test-InstalledBundle -BundlePath $backupBundle
            if ($backupCopy.TreeSha256 -ne $current.TreeSha256) { throw 'Update backup verification failed' }

            $state = [ordered]@{
                schema = 'skellspectre-backup/1'
                product = $script:ProductName
                created_utc = (Get-Date).ToUniversalTime().ToString('o')
                destination = $destinationFull
                previous_bundle = $script:BundleName
                previous_version = $current.Version
                previous_sha256 = $current.TreeSha256
                installed_version = $package.Version
                installed_sha256 = $package.TreeSha256
            }
            Write-JsonFile -Path (Join-Path $backupPath 'backup-manifest.json') -Value $state
            Move-Item -LiteralPath $target -Destination $oldMove
        }

        try {
            Move-Item -LiteralPath $stagedBundle -Destination $target
            $installed = Test-InstalledBundle -BundlePath $target
            if ($installed.TreeSha256 -ne $package.TreeSha256) { throw 'Installed VST3 hash does not match the verified package' }
        } catch {
            if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Recurse -Force }
            if (Test-Path -LiteralPath $oldMove -PathType Container) { Move-Item -LiteralPath $oldMove -Destination $target }
            throw
        }

        if (Test-Path -LiteralPath $oldMove -PathType Container) {
            Remove-Item -LiteralPath $oldMove -Recurse -Force
        }
        Write-Output "INSTALLED_VERSION=$($package.Version)"
        Write-Output "INSTALL_PATH=$target"
        if (-not [string]::IsNullOrWhiteSpace($backupPath)) { Write-Output "BACKUP_PATH=$backupPath" }
    } finally {
        if (Test-Path -LiteralPath $stageRoot) { Remove-Item -LiteralPath $stageRoot -Recurse -Force }
    }
}

function Invoke-RollbackInternal {
    param([Parameter(Mandatory)][object]$Plan)
    $id = [guid]::NewGuid().ToString('N')
    $stageRoot = Join-Path $Plan.Destination ".skell-spectre-rollback-$id"
    Ensure-SafeDirectory -Path $stageRoot
    $forwardBundle = Join-Path $Plan.BackupPath 'forward-current'
    try {
        if (Test-Path -LiteralPath $forwardBundle) { throw 'A forward-current bundle already exists in this rollback backup' }
        Copy-Item -LiteralPath $Plan.PreviousBundle -Destination $stageRoot -Recurse
        $stagedBundle = Join-Path $stageRoot $script:BundleName
        $staged = Test-InstalledBundle -BundlePath $stagedBundle
        if ($staged.TreeSha256 -ne $Plan.PreviousSha256) { throw 'Staged rollback bundle hash mismatch' }

        Move-Item -LiteralPath $Plan.Target -Destination $forwardBundle
        try {
            Move-Item -LiteralPath $stagedBundle -Destination $Plan.Target
            $restored = Test-InstalledBundle -BundlePath $Plan.Target
            if ($restored.TreeSha256 -ne $Plan.PreviousSha256) { throw 'Restored VST3 hash mismatch' }
        } catch {
            if (Test-Path -LiteralPath $Plan.Target) { Remove-Item -LiteralPath $Plan.Target -Recurse -Force }
            if (Test-Path -LiteralPath $forwardBundle -PathType Container) { Move-Item -LiteralPath $forwardBundle -Destination $Plan.Target }
            throw
        }

        $rollbackState = [ordered]@{
            schema = 'skellspectre-rollback/1'
            product = $script:ProductName
            rolled_back_utc = (Get-Date).ToUniversalTime().ToString('o')
            restored_version = $Plan.PreviousVersion
            restored_sha256 = $Plan.PreviousSha256
            forward_version = $Plan.CurrentVersion
            forward_sha256 = $Plan.CurrentSha256
            forward_bundle = 'forward-current'
        }
        Write-JsonFile -Path (Join-Path $Plan.BackupPath 'rollback-manifest.json') -Value $rollbackState
        Write-Output "ROLLED_BACK_VERSION=$($Plan.PreviousVersion)"
        Write-Output "INSTALL_PATH=$($Plan.Target)"
        Write-Output "ROLLBACK_FORWARD_PATH=$forwardBundle"
    } finally {
        if (Test-Path -LiteralPath $stageRoot) { Remove-Item -LiteralPath $stageRoot -Recurse -Force }
    }
}

if ([string]::IsNullOrWhiteSpace($Destination)) {
    if ([string]::IsNullOrWhiteSpace($env:LOCALAPPDATA)) { throw 'LOCALAPPDATA is unavailable; pass -Destination explicitly' }
    $Destination = Join-Path $env:LOCALAPPDATA 'Programs/Common/VST3'
}
if ([string]::IsNullOrWhiteSpace($BackupRoot)) {
    if ([string]::IsNullOrWhiteSpace($env:LOCALAPPDATA)) { throw 'LOCALAPPDATA is unavailable; pass -BackupRoot explicitly' }
    $BackupRoot = Join-Path $env:LOCALAPPDATA 'SkellAudio/Backups/SKELL SPECTRE'
}
$Destination = Get-FullPath $Destination
$BackupRoot = Get-FullPath $BackupRoot

if (-not [string]::IsNullOrWhiteSpace($RollbackBackup)) {
    if (-not [string]::IsNullOrWhiteSpace($PackageRoot)) {
        throw 'Do not combine -RollbackBackup with -PackageRoot'
    }
    $plan = Get-RollbackPlan -BackupRoot $BackupRoot -RollbackBackup $RollbackBackup -Destination $Destination
    if ($PSCmdlet.ShouldProcess($plan.Target, "Restore verified SKELL SPECTRE $($plan.PreviousVersion)")) {
        Invoke-RollbackInternal -Plan $plan
    } else {
        Write-Output 'Rollback validation passed. No files changed.'
    }
} else {
    if ([string]::IsNullOrWhiteSpace($PackageRoot)) { $PackageRoot = $PSScriptRoot }
    $package = Test-PreviewPackage -PackageRoot $PackageRoot
    Assert-SafeInstallPaths -PackageRoot $package.PackageRoot -Destination $Destination -BackupRoot $BackupRoot
    $target = Join-Path $Destination $script:BundleName
    if ($PSCmdlet.ShouldProcess($target, "Install validated SKELL SPECTRE $($package.Version) with verified backup/rollback")) {
        Invoke-InstallInternal -PackageRoot $PackageRoot -Destination $Destination -BackupRoot $BackupRoot
    } else {
        Write-Output 'Package validation passed. No files changed.'
    }
}
