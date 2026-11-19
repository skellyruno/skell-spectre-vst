#!/usr/bin/env python3
"""Stage a single, verified Windows VST3 preview without altering build output."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import struct
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

PRODUCT_NAME = "SKELL SPECTRE"
VENDOR_NAME = "Skell Audio"
BUNDLE_NAME = "SKELL SPECTRE.vst3"
BINARY_RELATIVE = Path("Contents/x86_64-win/SKELL SPECTRE.vst3")
MODULEINFO_RELATIVE = Path("Contents/Resources/moduleinfo.json")
EXPECTED_CLASSES = {"Audio Module Class", "Component Controller Class"}


class PackageError(RuntimeError):
    pass


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def strip_json_trailing_commas(source: str) -> str:
    """Remove commas before } or ] only when outside JSON strings."""
    output: list[str] = []
    in_string = False
    escaped = False
    index = 0
    while index < len(source):
        char = source[index]
        if in_string:
            output.append(char)
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                in_string = False
            index += 1
            continue
        if char == '"':
            in_string = True
            output.append(char)
            index += 1
            continue
        if char == ",":
            lookahead = index + 1
            while lookahead < len(source) and source[lookahead].isspace():
                lookahead += 1
            if lookahead < len(source) and source[lookahead] in "]}":
                index += 1
                continue
        output.append(char)
        index += 1
    return "".join(output)


def load_moduleinfo(path: Path) -> tuple[dict[str, Any], str, str]:
    original_bytes = path.read_bytes()
    original_hash = hashlib.sha256(original_bytes).hexdigest()
    try:
        original_text = original_bytes.decode("utf-8-sig")
        normalized_text = strip_json_trailing_commas(original_text)
        data = json.loads(normalized_text)
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise PackageError(f"moduleinfo.json is not valid UTF-8 JSON after JUCE comma normalization: {error}") from error
    if not isinstance(data, dict):
        raise PackageError("moduleinfo.json root must be an object")
    return data, normalized_text, original_hash


def project_version(repo_root: Path) -> str:
    cmake_path = repo_root / "CMakeLists.txt"
    if not cmake_path.is_file():
        raise PackageError(f"Missing project CMakeLists.txt: {cmake_path}")
    source = cmake_path.read_text(encoding="utf-8")
    match = re.search(
        r"(?im)^\s*project\s*\(\s*SkellSpectre\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)",
        source,
    )
    if not match:
        raise PackageError("Could not read the SkellSpectre project version from CMakeLists.txt")
    return match.group(1)


def validate_metadata(
    metadata: dict[str, Any], expected_version: str, require_commercial_metadata: bool = False
) -> list[str]:
    errors: list[str] = []
    if metadata.get("Name") != PRODUCT_NAME:
        errors.append(f"product name must be {PRODUCT_NAME!r}")
    if str(metadata.get("Version", "")) != expected_version:
        errors.append(f"module version must match CMake version {expected_version}")
    factory = metadata.get("Factory Info")
    if not isinstance(factory, dict):
        errors.append("Factory Info must be an object")
        factory = {}
    if factory.get("Vendor") != VENDOR_NAME:
        errors.append(f"vendor must be {VENDOR_NAME!r}")
    url = factory.get("URL")
    email = factory.get("E-Mail")
    if not isinstance(url, str):
        errors.append("Factory Info.URL must be a string")
    if not isinstance(email, str):
        errors.append("Factory Info.E-Mail must be a string")
    classes = metadata.get("Classes")
    if not isinstance(classes, list) or len(classes) != 2:
        errors.append("Classes must contain exactly the processor and controller entries")
        classes = classes if isinstance(classes, list) else []
    categories: list[str] = []
    cids: list[str] = []
    for item in classes:
        if not isinstance(item, dict):
            errors.append("every Classes entry must be an object")
            continue
        category = item.get("Category")
        if isinstance(category, str):
            categories.append(category)
        cid = item.get("CID")
        if isinstance(cid, str):
            cids.append(cid)
        else:
            errors.append("every class must have a string CID")
        if item.get("Name") != PRODUCT_NAME:
            errors.append("every class name must match the product name")
        if item.get("Vendor") != VENDOR_NAME:
            errors.append("every class vendor must match the factory vendor")
        if str(item.get("Version", "")) != expected_version:
            errors.append("every class version must match the CMake version")
    if set(categories) != EXPECTED_CLASSES:
        errors.append("class categories must be one Audio Module Class and one Component Controller Class")
    if len(cids) != len(set(cids)):
        errors.append("class CIDs must be unique")
    if any(not re.fullmatch(r"[0-9A-Fa-f]{32}", cid) for cid in cids):
        errors.append("class CIDs must be 32 hexadecimal characters")
    if require_commercial_metadata:
        if not isinstance(url, str) or not url.strip():
            errors.append("commercial release requires a non-empty vendor URL")
        if not isinstance(email, str) or not email.strip():
            errors.append("commercial release requires a non-empty support email")
    return errors


def validate_commercial_files(repo_root: Path) -> list[str]:
    required = ["LICENSE", "THIRD_PARTY_NOTICES.txt", "EULA.txt"]
    errors = [f"commercial release requires {name}" for name in required if not (repo_root / name).is_file()]
    notices = repo_root / "THIRD_PARTY_NOTICES.txt"
    if notices.is_file() and "STATUS: COMPLETE" not in notices.read_text(encoding="utf-8"):
        errors.append("THIRD_PARTY_NOTICES.txt must be reviewed and marked STATUS: COMPLETE")
    return errors


def validate_pe_x64(path: Path) -> dict[str, Any]:
    data = path.read_bytes()
    if len(data) < 0x40 or data[:2] != b"MZ":
        raise PackageError(f"VST3 module is not a valid PE executable: {path}")
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if pe_offset + 26 > len(data) or data[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise PackageError(f"VST3 module has an invalid PE header: {path}")
    machine = struct.unpack_from("<H", data, pe_offset + 4)[0]
    optional_magic = struct.unpack_from("<H", data, pe_offset + 24)[0]
    if machine != 0x8664:
        raise PackageError(f"VST3 module must be AMD64 (0x8664), got 0x{machine:04x}")
    if optional_magic != 0x20B:
        raise PackageError(f"VST3 module must be PE32+ (0x20b), got 0x{optional_magic:04x}")
    return {"machine": "AMD64", "machine_code": f"0x{machine:04x}", "optional_header": "PE32+"}


def find_bundle(build_root: Path) -> Path:
    if not build_root.is_dir():
        raise PackageError(f"Build output directory does not exist: {build_root}")
    bundles = sorted(path for path in build_root.rglob("*.vst3") if path.is_dir())
    if len(bundles) != 1:
        raise PackageError(f"expected exactly one VST3 bundle under {build_root}, found {len(bundles)}")
    if bundles[0].name != BUNDLE_NAME:
        raise PackageError(f"unexpected VST3 bundle name: {bundles[0].name!r}")
    return bundles[0]


def file_manifest(root: Path) -> list[dict[str, Any]]:
    entries = []
    for path in sorted(item for item in root.rglob("*") if item.is_file()):
        entries.append({
            "path": path.relative_to(root).as_posix(),
            "size_bytes": path.stat().st_size,
            "sha256": sha256_file(path),
        })
    return entries


def package_preview(
    build_root: Path, output_dir: Path, repo_root: Path, source_commit: str,
    ref_name: str, run_id: str, require_commercial_metadata: bool = False,
    ci_checkout_commit: str = "local-unpinned",
) -> dict[str, Any]:
    bundle = find_bundle(build_root)
    source_moduleinfo = bundle / MODULEINFO_RELATIVE
    source_binary = bundle / BINARY_RELATIVE
    if not source_moduleinfo.is_file():
        raise PackageError(f"missing VST3 moduleinfo: {source_moduleinfo}")
    if not source_binary.is_file():
        raise PackageError(f"missing Windows x64 VST3 module: {source_binary}")
    version = project_version(repo_root)
    metadata, _, original_metadata_hash = load_moduleinfo(source_moduleinfo)
    errors = validate_metadata(metadata, version, require_commercial_metadata)
    if require_commercial_metadata:
        errors.extend(validate_commercial_files(repo_root))
    if errors:
        raise PackageError("metadata gate failed:\n - " + "\n - ".join(errors))
    pe_info = validate_pe_x64(source_binary)
    if output_dir.exists():
        raise PackageError(f"refusing to overwrite existing staging directory: {output_dir}")
    output_dir.mkdir(parents=True)
    staged_bundle = output_dir / BUNDLE_NAME
    shutil.copytree(bundle, staged_bundle)
    staged_moduleinfo = staged_bundle / MODULEINFO_RELATIVE
    staged_moduleinfo.write_text(json.dumps(metadata, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    staged_binary = staged_bundle / BINARY_RELATIVE
    binary_hash = sha256_file(staged_binary)
    normalized_metadata_hash = sha256_file(staged_moduleinfo)
    factory = metadata.get("Factory Info", {})
    missing_commercial_fields = []
    if not str(factory.get("URL", "")).strip():
        missing_commercial_fields.append("vendor URL")
    if not str(factory.get("E-Mail", "")).strip():
        missing_commercial_fields.append("support email")
    missing_commercial_fields.extend(
        name for name in ("LICENSE", "THIRD_PARTY_NOTICES.txt", "EULA.txt") if not (repo_root / name).is_file()
    )
    notices_path = repo_root / "THIRD_PARTY_NOTICES.txt"
    if notices_path.is_file() and "STATUS: COMPLETE" not in notices_path.read_text(encoding="utf-8"):
        missing_commercial_fields.append("third-party notices incomplete")
    manifest = {
        "product": PRODUCT_NAME,
        "vendor": VENDOR_NAME,
        "version": version,
        "package_type": "windows-x64-vst3-preview",
        "release_status": "PREVIEW_ONLY",
        "source_commit": source_commit,
        "ci_checkout_commit": ci_checkout_commit,
        "ref_name": ref_name,
        "workflow_run_id": run_id,
        "pluginval": {"version": "1.0.4", "strictness_level": 5, "result": "pending"},
        "platform": "Windows x64",
        "binary_path": (Path(BUNDLE_NAME) / BINARY_RELATIVE).as_posix(),
        "binary_size_bytes": staged_binary.stat().st_size,
        "binary_sha256": binary_hash,
        "moduleinfo_path": (Path(BUNDLE_NAME) / MODULEINFO_RELATIVE).as_posix(),
        "moduleinfo_original_sha256": original_metadata_hash,
        "moduleinfo_normalized_sha256": normalized_metadata_hash,
        "moduleinfo_normalization": "JUCE trailing commas removed in staged copy only",
        "architecture": pe_info,
        "commercial_release_ready": not missing_commercial_fields,
        "commercial_release_blockers": missing_commercial_fields,
        "runtime_acceptance": "UNVERIFIED: FL Studio and physical Windows host acceptance are separate gates",
        "audio_acceptance": "UNVERIFIED: real audio A/B, CPU and latency acceptance are separate gates",
        "created_utc": datetime.now(timezone.utc).isoformat(),
    }
    (output_dir / "BUILD-MANIFEST.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    (output_dir / "PREVIEW-ONLY.txt").write_text(
        "SKELL SPECTRE — PRIVATE WINDOWS VST3 PREVIEW\n\n"
        "This package is for private testing only. It is not a commercial release and must not be resold or redistributed.\n"
        "No public release, code signing, commercial licence approval, vendor contact approval, or FL Studio runtime acceptance is implied.\n"
        "The included build manifest records the exact source commit and binary checksums.\n",
        encoding="utf-8",
    )
    preview_resources = (
        (Path("tools/Install-SKELL-SPECTRE-Preview.ps1"), "Install-SKELL-SPECTRE-Preview.ps1"),
        (Path("docs/WINDOWS-VST3-INSTALL.md"), "WINDOWS-VST3-INSTALL.md"),
        (Path("THIRD_PARTY_NOTICES.txt"), "THIRD_PARTY_NOTICES.txt"),
    )
    for source_relative, destination_name in preview_resources:
        source_path = repo_root / source_relative
        if not source_path.is_file():
            raise PackageError(f"Missing private-preview package resource: {source_path}")
        shutil.copy2(source_path, output_dir / destination_name)

    entries = file_manifest(output_dir)
    (output_dir / "SHA256SUMS.txt").write_text(
        "".join(f"{entry['sha256']}  {entry['path']}\n" for entry in entries), encoding="utf-8"
    )
    staged_bundles = [path for path in output_dir.rglob("*.vst3") if path.is_dir()]
    if len(staged_bundles) != 1 or staged_bundles[0].name != BUNDLE_NAME:
        raise PackageError("staged package must contain exactly one VST3 bundle")
    forbidden = [
        path for path in output_dir.rglob("*")
        if path.is_file() and path.suffix.lower() in {".exe", ".component", ".app"}
    ]
    if forbidden:
        raise PackageError("non-VST3 payload found in preview package: " + ", ".join(str(path) for path in forbidden))
    json.loads(staged_moduleinfo.read_text(encoding="utf-8"))
    validate_pe_x64(staged_binary)
    return manifest


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--build-root", type=Path, default=Path("build/SkellSpectre_artefacts"))
    parser.add_argument("--output-dir", type=Path, default=Path("release-stage"))
    parser.add_argument("--source-commit", default=None)
    parser.add_argument("--ref-name", default=None)
    parser.add_argument("--run-id", default=None)
    parser.add_argument("--ci-checkout-commit", default=None)
    parser.add_argument("--require-commercial-metadata", action="store_true")
    args = parser.parse_args(argv)
    repo_root = args.repo_root.resolve()
    build_root = args.build_root if args.build_root.is_absolute() else (repo_root / args.build_root)
    output_dir = args.output_dir if args.output_dir.is_absolute() else (repo_root / args.output_dir)
    try:
        manifest = package_preview(
            build_root=build_root, output_dir=output_dir, repo_root=repo_root,
            source_commit=args.source_commit or os.environ.get("GITHUB_SHA", "local-unpinned"),
            ref_name=args.ref_name or os.environ.get("GITHUB_REF_NAME", "local"),
            run_id=args.run_id or os.environ.get("GITHUB_RUN_ID", "local"),
            require_commercial_metadata=args.require_commercial_metadata,
            ci_checkout_commit=args.ci_checkout_commit or os.environ.get(
                "CI_CHECKOUT_COMMIT", os.environ.get("GITHUB_SHA", "local-unpinned")
            ),
        )
    except (PackageError, OSError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    print(f"STAGED: {output_dir}")
    print(f"PRODUCT: {manifest['product']} {manifest['version']}")
    print(f"BINARY SHA-256: {manifest['binary_sha256']}")
    print(f"COMMERCIAL RELEASE READY: {manifest['commercial_release_ready']}")
    if manifest["commercial_release_blockers"]:
        print("COMMERCIAL BLOCKERS: " + "; ".join(manifest["commercial_release_blockers"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
