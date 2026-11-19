# SKELL SPECTRE — Windows VST3 private-preview install

This is a private preview, not a commercial release. Keep the package private. Do not resell, redistribute, or use it commercially.

## Requirements

- Windows x64
- PowerShell 7
- A VST3-compatible host
- The complete extracted preview folder, including BUILD-MANIFEST.json, SHA256SUMS.txt, PREVIEW-ONLY.txt, the installer, and the SKELL SPECTRE.vst3 bundle

## Validate before changing files

Open PowerShell 7 in the extracted package folder and run:

    pwsh -NoProfile -File .\Install-SKELL-SPECTRE-Preview.ps1 -PackageRoot (Get-Location).Path -WhatIf

The script verifies the package inventory, SHA-256 checksums, exact source and CI commit provenance, pluginval strictness-5 result, VST3 product/vendor/version, preserved class IDs, and AMD64 PE32+ module. WhatIf must not create or modify the install or backup directories.

## Install or update

    pwsh -NoProfile -File .\Install-SKELL-SPECTRE-Preview.ps1 -PackageRoot (Get-Location).Path -Confirm

The default per-user destination is:

    %LOCALAPPDATA%\Programs\Common\VST3\SKELL SPECTRE.vst3

No administrator elevation, registry changes, machine-wide writes, or automatic updater are used. If an older SKELL SPECTRE bundle is already installed, the script verifies its identity, preserves it under the per-user SkellAudio backup folder, stages the new bundle, verifies the staged copy, and then swaps it into place. A different plugin or an unrecognized file at the destination is never overwritten.

The installer prints BACKUP_PATH=... when an update backup is created. Keep that exact folder if you may need to roll back.

## Roll back an update

Use the exact backup path printed by the installer:

    pwsh -NoProfile -File .\Install-SKELL-SPECTRE-Preview.ps1 -RollbackBackup "C:\Users\YOU\AppData\Local\SkellAudio\Backups\SKELL SPECTRE\BACKUP-ID" -Confirm

Rollback verifies the backup manifest and both bundle hashes. It refuses to overwrite the current installation if its files have changed since the update. The newer bundle is preserved in forward-current inside the backup folder.

## Important release limits

- This package is unsigned and preview-only.
- Vendor URL, support email, final EULA, third-party license/asset attribution, commercial approval, FL Studio host acceptance, musical A/B, and target-hardware CPU/latency remain separate gates.
- The included third-party notice file is explicitly incomplete; it is not a redistribution or commercial-use license.
- This installer does not publish a release, take payment, deploy a website, or configure an updater.
