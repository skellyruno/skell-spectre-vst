# SKELL SPECTRE — Windows VST3 release checklist

This checklist separates a **private preview** from a commercial release. A green GitHub build is not FL Studio acceptance and does not approve the product for sale.

## Windows preview pipeline

- [x] Windows x64 VST3 build and CTest job exists.
- [x] Portable DSP sanitizer job exists (Clang ASan/UBSan).
- [ ] Pinned pluginval 1.0.4 strictness-5 run on the current Windows VST3 (archive SHA-256 pinned in the workflow).
- [ ] Package exactly one `SKELL SPECTRE.vst3` bundle and its `Contents/x86_64-win` AMD64 PE32+ module.
- [ ] Normalize JUCE trailing commas in a staged `moduleinfo.json` copy only; never rewrite the build output.
- [ ] Record source commit, workflow run, binary hash, metadata hash and per-file SHA-256 checksums.
- [ ] Exclude Standalone EXE, AU and other formats from the Windows preview artifact.
- [ ] Retain the preview-only notice until the owner approves commercial release decisions.

## Current baseline evidence

- Main merge commit: `fb34518e0bda6e9c4285e04b692db59ef0bc204a`.
- Windows build/test artifact run: `43783047049` (PR head `6b134e3c1d02286d9a82924f009aa78f061cb35d`, later merged to main).
- Baseline Windows artifact ID: `14189522302`; outer artifact SHA-256 `70338f0428b51c6da9b6f7c966ef772e593cb7ee6ce9ceeab7c39ba5e79c3d60`.
- Inner ZIP SHA-256: `1778f43803d8fadd76eb6436d7284161e2f4581d258133918193d9caedff6b50`.
- VST3 module SHA-256: `6f90e687a72e200e9abb2cc8a37098c5089fd706fa165677a76d9511de47395f`.
- The baseline bundle contains one x64 PE32+ VST3 module. Its JUCE-generated `moduleinfo.json` has trailing commas and is rejected by strict JSON parsing; the new packager fixes only the staged copy.

## Commercial release blockers

The current module metadata has an empty vendor URL and support email. The repository also has no `LICENSE`, `THIRD_PARTY_NOTICES.txt`, or `EULA.txt` at its root. Do not invent these values or publish a commercial build until the owner has approved the legal and support details.

The packager allows a private preview but fails closed for version-tag builds until those fields and files exist.

## Independent acceptance still required

- [ ] FL Studio scan, insert, editor open/close/reopen, automation and project save/reload.
- [ ] Real audio A/B, finite-output/NaN/denormal/clipping/glitch checks, latency and CPU on target Windows hardware.
- [ ] Preset and A/B state recall across save/reopen; backward compatibility with existing projects.
- [ ] Manual keyboard/focus/popup and high-DPI interaction checks.
- [ ] Licence, attribution, trademark, support/contact and pricing/trial approval.
- [ ] Installer signing, private download access, update/rollback and website storefront coordination.

The existing actual-editor screenshot workflow is separate from this release workflow. It must remain a required visual regression gate, but its captures do not replace the FL Studio checks above.
