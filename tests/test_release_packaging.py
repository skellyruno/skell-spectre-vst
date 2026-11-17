from __future__ import annotations

import hashlib
import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.package_windows_vst3 import (  # noqa: E402
    BINARY_RELATIVE, BUNDLE_NAME, MODULEINFO_RELATIVE, PackageError,
    load_moduleinfo, package_preview, strip_json_trailing_commas,
    validate_metadata, validate_pe_x64,
)


def make_pe_x64() -> bytes:
    data = bytearray(512)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x80)
    data[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", data, 0x84, 0x8664)
    struct.pack_into("<H", data, 0x98, 0x20B)
    return bytes(data)


def metadata(version="1.8.3", url="", email=""):
    return {
        "Name": "SKELL SPECTRE", "Version": version,
        "Factory Info": {"Vendor": "Skell Audio", "URL": url, "E-Mail": email},
        "Classes": [
            {"CID": "ABCDEF019182FAEB536B417553706374", "Category": "Audio Module Class",
             "Name": "SKELL SPECTRE", "Vendor": "Skell Audio", "Version": version},
            {"CID": "ABCDEF011234ABCD536B417553706374", "Category": "Component Controller Class",
             "Name": "SKELL SPECTRE", "Vendor": "Skell Audio", "Version": version},
        ],
    }


def fixture(root: Path, trailing_commas=False):
    (root / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.22)\nproject(SkellSpectre VERSION 1.8.3 LANGUAGES C CXX)\n",
        encoding="utf-8",
    )
    bundle = root / "build" / "SkellSpectre_artefacts" / "Release" / BUNDLE_NAME
    (bundle / MODULEINFO_RELATIVE).parent.mkdir(parents=True, exist_ok=True)
    (bundle / BINARY_RELATIVE).parent.mkdir(parents=True, exist_ok=True)
    text = json.dumps(metadata(), indent=2)
    if trailing_commas:
        text = text.replace('"E-Mail": ""', '"E-Mail": "",')
        text = text.replace('"Version": "1.8.3"\n  }', '"Version": "1.8.3",\n  }')
    (bundle / MODULEINFO_RELATIVE).write_text(text, encoding="utf-8")
    (bundle / BINARY_RELATIVE).write_bytes(make_pe_x64())
    return root / "build" / "SkellSpectre_artefacts"


class ReleasePackagingTests(unittest.TestCase):
    def test_trailing_comma_removal_ignores_string_contents(self):
        source = '{"value": "literal ,] text", "items": [1, 2,],}'
        normalized = strip_json_trailing_commas(source)
        self.assertEqual(json.loads(normalized), {"value": "literal ,] text", "items": [1, 2]})

    def test_moduleinfo_loads_juce_trailing_commas(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "moduleinfo.json"
            path.write_text('{"Name":"SKELL SPECTRE",}', encoding="utf-8")
            parsed, normalized, original_hash = load_moduleinfo(path)
            self.assertEqual(parsed["Name"], "SKELL SPECTRE")
            self.assertNotIn(",}", normalized)
            self.assertEqual(len(original_hash), 64)

    def test_metadata_accepts_preview_but_flags_empty_contact(self):
        self.assertEqual(validate_metadata(metadata(), "1.8.3"), [])
        errors = validate_metadata(metadata(), "1.8.3", require_commercial_metadata=True)
        self.assertTrue(any("vendor URL" in error for error in errors))
        self.assertTrue(any("support email" in error for error in errors))

    def test_metadata_rejects_wrong_version_and_duplicate_cids(self):
        bad = metadata(version="9.9.9")
        bad["Classes"][1]["CID"] = bad["Classes"][0]["CID"]
        errors = validate_metadata(bad, "1.8.3")
        self.assertTrue(any("CMake version" in error for error in errors))
        self.assertTrue(any("unique" in error for error in errors))

    def test_pe_requires_amd64_pe32_plus(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "plugin.vst3"
            path.write_bytes(make_pe_x64())
            self.assertEqual(validate_pe_x64(path)["machine"], "AMD64")
            bad = bytearray(make_pe_x64())
            struct.pack_into("<H", bad, 0x84, 0x14C)
            path.write_bytes(bad)
            with self.assertRaises(PackageError):
                validate_pe_x64(path)

    def test_package_normalizes_only_staged_copy_and_generates_checksums(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            build_root = fixture(root, trailing_commas=True)
            source_bundle = next(path for path in build_root.rglob("*.vst3") if path.is_dir())
            original_moduleinfo = (source_bundle / MODULEINFO_RELATIVE).read_bytes()
            output = root / "release-stage"
            manifest = package_preview(build_root, output, root, "abc123", "test", "42")
            staged = output / BUNDLE_NAME
            self.assertEqual((source_bundle / MODULEINFO_RELATIVE).read_bytes(), original_moduleinfo)
            json.loads((staged / MODULEINFO_RELATIVE).read_text(encoding="utf-8"))
            self.assertEqual(manifest["release_status"], "PREVIEW_ONLY")
            self.assertTrue((output / "BUILD-MANIFEST.json").is_file())
            self.assertTrue((output / "SHA256SUMS.txt").is_file())
            self.assertTrue((output / "PREVIEW-ONLY.txt").is_file())
            self.assertFalse(any(path.suffix.lower() == ".exe" for path in output.rglob("*")))
            for line in (output / "SHA256SUMS.txt").read_text(encoding="utf-8").splitlines():
                digest, relative = line.split("  ", 1)
                self.assertEqual(hashlib.sha256((output / relative).read_bytes()).hexdigest(), digest)

    def test_package_refuses_duplicate_bundles(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            build_root = fixture(root)
            (build_root / "Duplicate.vst3").mkdir()
            with self.assertRaises(PackageError):
                package_preview(build_root, root / "out", root, "abc", "test", "1")

    def test_commercial_mode_fails_closed_without_contact_and_legal_files(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            build_root = fixture(root)
            with self.assertRaises(PackageError) as context:
                package_preview(build_root, root / "out", root, "abc", "v1.8.3", "1", True)
            self.assertIn("vendor URL", str(context.exception))
            self.assertIn("LICENSE", str(context.exception))


if __name__ == "__main__":
    unittest.main(verbosity=2)
