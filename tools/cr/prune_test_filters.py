#!/usr/bin/env vpython3
#
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
#
# prune_test_filters.py
#
# Safely detect and optionally remove obsolete test filters in Brave. Only
# filters for the current platform are processed; others are skipped.
#
# Arguments:
#   --build-dir <path>  : Directory containing test binaries (required)
#   --apply             : Actually remove obsolete tests from filter files (optional)
#   --sort              : Sort the filters within each section (only valid with --apply)
#

import argparse
import subprocess
import sys
import fnmatch
from pathlib import Path
from collections import defaultdict

SCRIPT_DIR = Path(__file__).resolve().parent
BRAVE_ROOT = SCRIPT_DIR.parent.parent
FILTER_DIR = BRAVE_ROOT / "test" / "filters"

# Filters listed here (with or without *) are never removed
PROTECTED_FILTERS = [
    "All/ProfilePickerCreationFlowEphemeralProfileBrowserTest.ExitDuringSignin/*",
    "All/ProfilePickerCreationFlowEphemeralProfileBrowserTest.Signin/*",
    "BorderlessIsolatedWebAppBrowserTest.*",
    "BorderlessIsolatedWebAppBrowserTestDisabledFlag.AppCannotUseFeatureWhenBorderlessFlagIsDisabled",
    "BrowserInstantControllerTest.DefaultSearchProviderChanged",
    "FedCmAccountSelectionViewBrowserTest.*",
    "FirstPartySetsBrowserTestWithSiteLeavingSet.CookieDeleted",
    "GlicUserStatusBrowserTest.ClientDataHeaderExists",
    "GPMPasskeysAuthenticatorDialogTest.InvokeUi*",
    "HintsFetcherSearchPageLimitedURLsBrowserTest.HintsFetcherLimitedResults",
    "HistorySyncOptinManagedType/AvatarToolbarButtonHistorySyncOptinManagedTypeTest.HistorySyncOptinNotShownWhenSyncManaged/*",
    "Incognito/EnclaveAuthenticatorIncognitoBrowserTest.MultipleDeclinedBootstrappings/*",
    "ManagedProfileCreationBrowserTest.Test/*PrimaryAccount_*",
    "ModelExecutionValidationBrowserTest.ModelExecutionFailsServerFailure",
    "ModelExecutionValidationBrowserTest.ModelExecutionSuccess",
    "NTPTilesForSupervisedUsersTest.DoNotLoadBlockedURL",
    "OmniboxContextMenuControllerBrowserTest.ExecuteCommand",
    "PageLoadMetricsBrowserTestWithFencedFrames.PageLoadPrivacySandboxAdsFencedFramesMetrics",
    "ProfileMenuViewBookmarksLimitExceededTest.ResolveBookmarksLimitExceededError/kSyncTheFeature",
    "ProfilePickerGlicFlowControllerBrowserTest.PickProfileWithCurrentProfile",
    "RustLogIntegrationTest.CheckAllSeverity",
    "SidePanelCoordinatorTest.ClosingMidShowFromAnimationReparentsContentView",
    "TabSearchButtonBrowserTest.ButtonClickCreatesBubble",
    "TabSearchButtonBrowserUITest.InvokeUi_default",
    "WebAppInternalsIwaInstallationBrowserTest.*",
]


def get_current_platform_tag():
    """Return normalized platform tag: mac, linux, win, or unknown"""
    if sys.platform.startswith("darwin"):
        return "mac"
    if sys.platform.startswith("linux"):
        return "linux"
    if sys.platform.startswith("win"):
        return "win"
    return "unknown"


def is_filter_applicable(filename, current_platform):
    """
    Determine if a filter file applies to the current platform.
    The platform/variant info is everything after the first '-' and before '.filter'.
    If it starts with a platform we don't support, skip the file.
    """
    name = filename.name
    if not name.endswith(".filter"):
        return False

    base = name[:-len(".filter")]

    if "-" not in base:
        return True

    platform_info = base.split("-", 1)[1].lower()

    supported_platforms = {"mac": "mac", "linux": "linux", "windows": "win"}

    for plat_key, plat_tag in supported_platforms.items():
        if platform_info.startswith(plat_key):
            if plat_tag == current_platform:
                return True
            print(
                f"Skipping {filename} (platform '{plat_key}' does not match '{current_platform}')"
            )
            return False

    print(
        f"Warning: skipping {filename}, unknown platform pattern '{platform_info}'"
    )
    return False


def get_binary_name_from_filter(filename):
    """Extract test binary name from a filter filename."""
    name = filename.name
    if not name.endswith(".filter"):
        return None
    base = name[:-len(".filter")]
    if "-" in base:
        base = base.split("-")[0]
    return base


def find_binary(build_dir, binary_name):
    """Find and return the test binary path in the build directory."""
    candidates = [binary_name]
    if sys.platform.startswith("win"):
        candidates.insert(0, binary_name + ".exe")
    for candidate in candidates:
        path = build_dir / candidate
        if path.exists():
            return path
    return None


def list_tests_from_binary(binary_path):
    """Return set of test names from a gtest binary."""
    try:
        output = subprocess.check_output(
            [str(binary_path), "--gtest_list_tests"],
            stderr=subprocess.DEVNULL,
            text=True,
            encoding="utf-8",
            errors="ignore",
        )
    except Exception as e:
        print(f"Warning: failed to run {binary_path}: {e}", file=sys.stderr)
        return set()

    tests = set()
    current_suite = ""

    for line in output.splitlines():
        stripped = line.strip()
        if not stripped:
            continue

        if not line.startswith(" "):
            suite_part = stripped.split("#", 1)[0].strip().rstrip(".")
            if suite_part:
                current_suite = suite_part
            continue

        main_part = stripped.split("#", 1)[0].strip().rstrip(".")
        if not main_part:
            continue

        full_name = f"{current_suite}.{main_part}" if current_suite else main_part
        tests.add(full_name)

    return tests


def collect_tests_per_binary(build_dir, current_platform):
    """
    Collect tests per binary.

    Args:
        build_dir: Path to build directory with test binaries.
        current_platform: Platform tag (mac/linux/win).

    Returns:
        tests_by_binary: Dict mapping binary_name -> set of test names.
    """
    tests_by_binary = {}
    seen_binaries = set()

    for path in FILTER_DIR.rglob("*.filter"):
        if not is_filter_applicable(path, current_platform):
            continue

        binary_name = get_binary_name_from_filter(path)
        if not binary_name:
            sys.stderr.write(
                f"Error: could not determine binary from filter file: {path}\n"
            )
            sys.exit(1)

        if binary_name in seen_binaries:
            continue
        seen_binaries.add(binary_name)

        binary_path = find_binary(build_dir, binary_name)
        if not binary_path:
            sys.stderr.write(
                f"Error: binary '{binary_name}' referenced by {path} was not "
                "found in build directory\n")
            sys.exit(1)

        tests = list_tests_from_binary(binary_path)
        tests_by_binary[binary_name] = tests

    return tests_by_binary


def scan_filter_files(tests_by_binary,
                      current_platform,
                      apply=False,
                      sort=False):
    """Find (and optionally remove) obsolete filters.

    Args:
        tests_by_binary: Map of binary -> set of valid tests.
        current_platform: Platform tag (mac/linux/win).
        apply: If True, modify files in-place.
        sort: If True, sort filters (requires apply).

    Returns:
        obsolete_by_file map.
    """
    obsolete_by_file = defaultdict(list)

    for path in FILTER_DIR.rglob("*.filter"):
        if not is_filter_applicable(path, current_platform):
            continue

        binary_name = get_binary_name_from_filter(path)
        if not binary_name or binary_name not in tests_by_binary:
            continue
        valid_tests = tests_by_binary[binary_name]

        with path.open("r", newline="") as f:
            content = f.read()

        lines = content.splitlines()
        new_lines = []
        changed = False

        file_header = []
        seen_header_end = False
        header_blank_line_preserved = False

        section_header = []
        section_filters = []
        section_has_kept_filter = False

        def flush_section():
            """Write current section, applying filtering and sorting rules."""
            nonlocal section_header, section_filters, section_has_kept_filter, new_lines
            if not section_header and not section_filters:
                return

            if section_has_kept_filter or not apply:
                new_lines.extend(section_header)
                filters_to_write = section_filters
                if sort and apply and section_has_kept_filter:
                    filters_to_write = sorted(section_filters,
                                              key=lambda l: l.strip())
                new_lines.extend(filters_to_write)
                new_lines.append("")

            section_header.clear()
            section_filters.clear()
            section_has_kept_filter = False

        for line in lines:
            raw = line.strip()

            # Preserve top-of-file header and blank line
            if not seen_header_end:
                if raw.startswith("#"):
                    file_header.append(line)
                    continue
                new_lines.extend(file_header)
                if lines[len(file_header)].strip() == "":
                    new_lines.append("")
                    header_blank_line_preserved = True
                seen_header_end = True

            if not raw:
                flush_section()
                continue

            if raw.startswith("#"):
                if section_filters:
                    flush_section()
                section_header.append(line)
                continue

            no_comment = raw.split("#", 1)[0].strip()

            if not no_comment.startswith("-"):
                flush_section()
                new_lines.append(line)
                continue

            filter_pattern = no_comment[1:].strip()
            if not filter_pattern:
                section_filters.append(line)
                continue

            protected = any(
                fnmatch.fnmatch(filter_pattern, p)
                or fnmatch.fnmatch(p, filter_pattern)
                for p in PROTECTED_FILTERS)

            matched = any(
                fnmatch.fnmatch(test, filter_pattern) for test in valid_tests)

            if matched or protected:
                section_filters.append(line)
                section_has_kept_filter = True
            else:
                obsolete_by_file[path].append(raw)
                changed = True
                if not apply:
                    section_filters.append(line)

        flush_section()

        while new_lines and new_lines[-1] == "":
            if header_blank_line_preserved and len(
                    new_lines) == len(file_header) + 1:
                break
            new_lines.pop()

        # Always write with LF line endings for consistency (BS-029)
        if apply and changed:
            with path.open("w", newline="\n") as f:
                f.write("\n".join(new_lines) + "\n")

    return obsolete_by_file


def main():
    """Parse args, collect tests, scan filters, and report results."""
    parser = argparse.ArgumentParser(
        description="List or prune obsolete test filters.")
    parser.add_argument("--build-dir",
                        required=True,
                        help="Directory with test binaries")
    parser.add_argument(
        "--apply",
        action="store_true",
        help="Remove obsolete tests from filter files (default is dry-run)",
    )
    parser.add_argument(
        "--sort",
        action="store_true",
        help="Sort the filters within each section (only valid with --apply)",
    )

    args = parser.parse_args()

    if args.sort and not args.apply:
        sys.stderr.write("Error: --sort requires --apply\n")
        sys.exit(1)

    build_dir = Path(args.build_dir)

    if not build_dir.exists():
        sys.stderr.write(
            f"Error: build directory does not exist: {build_dir}\n")
        sys.exit(1)

    if not FILTER_DIR.exists():
        sys.stderr.write(
            f"Error: filter directory does not exist: {FILTER_DIR}\n")
        sys.exit(1)

    current_platform = get_current_platform_tag()
    if current_platform == "unknown":
        sys.stderr.write(
            "Error: unknown platform, cannot determine current platform tag\n")
        sys.exit(1)

    tests_by_binary = collect_tests_per_binary(build_dir, current_platform)

    obsolete_by_file = scan_filter_files(tests_by_binary,
                                         current_platform,
                                         apply=args.apply,
                                         sort=args.sort)

    if obsolete_by_file:
        print("\nObsolete test filters:")
        for path, tests in obsolete_by_file.items():
            print(f"\nFile: {path}")
            for t in tests:
                print(f"  {t}")
    else:
        print("\nNo obsolete filters found.")

    if args.apply and obsolete_by_file:
        print("\nFilter files updated.")


if __name__ == "__main__":
    main()
