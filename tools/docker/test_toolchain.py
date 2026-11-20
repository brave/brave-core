# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

"""Verify checkout-specific Node/pnpm selection without host installation."""

import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import create_container as tools


class ToolchainTests(unittest.TestCase):
    def package(self, node=">=24.16.0 <27.0.0", pnpm=">=11.11.0"):
        return {
            "devEngines": {
                "runtime": {"name": "node", "version": node},
                "packageManager": {"name": "pnpm", "version": pnpm},
            }
        }

    def test_current_checkout_selects_minimum_versions(self):
        self.assertEqual(
            tools.toolchain_versions(self.package()),
            {"node": "24.16.0", "pnpm": "11.11.0"},
        )

    def test_another_checkout_uses_legacy_engines(self):
        self.assertEqual(
            tools.toolchain_versions(
                {"engines": {"node": ">=22.16.0 <23.0.0", "pnpm": "10.11.0"}}
            ),
            {"node": "22.16.0", "pnpm": "10.11.0"},
        )

    def test_development_engines_override_legacy_values(self):
        package = self.package()
        package["engines"] = {"node": "22.16.0", "pnpm": "10.11.0"}
        self.assertEqual(tools.toolchain_versions(package)["node"], "24.16.0")

    def test_exact_pnpm_pin_overrides_range_and_accepts_integrity_hash(self):
        package = self.package(pnpm=">=10.11.0 || >=11.11.0")
        package["packageManager"] = "pnpm@11.12.0+sha512.abcdef"
        self.assertEqual(tools.toolchain_versions(package)["pnpm"], "11.12.0")

    def test_incompatible_or_wrong_package_manager_pin_is_rejected(self):
        for pin in ("pnpm@10.11.0", "npm@11.11.0", "pnpm@latest"):
            with self.subTest(pin=pin), self.assertRaises(RuntimeError):
                tools.toolchain_versions(
                    {**self.package(), "packageManager": pin}
                )

    def test_range_operators_and_alternatives(self):
        for requirement, expected in (
            ("24.16.0", "24.16.0"),
            (">24.16.0 <=24.16.1", "24.16.1"),
            ("^24.16.0", "24.16.0"),
            ("~24.16.0", "24.16.0"),
            (">=24.16.0 <24.16.0 || >=25.1.0", "25.1.0"),
            ("^0.2.3 >=0.2.5", "0.2.5"),
        ):
            with self.subTest(requirement=requirement):
                self.assertEqual(tools.minimum_version(requirement), expected)

    def test_invalid_missing_or_empty_requirements_fail_before_installation(
        self,
    ):
        for package in (
            {},
            self.package(node="latest"),
            self.package(node=">=24.16.0 <24.16.0"),
            self.package(node="24.x"),
        ):
            with (
                self.subTest(package=package),
                tempfile.TemporaryDirectory() as directory,
            ):
                manifest = Path(directory) / "package.json"
                manifest.write_text(json.dumps(package), encoding="utf-8")
                with (
                    patch.object(tools.urllib.request, "urlopen") as download,
                    patch.object(tools.subprocess, "run") as run,
                ):
                    with self.assertRaises(RuntimeError):
                        tools.install_toolchain(manifest)
                    download.assert_not_called()
                    run.assert_not_called()

    def test_matching_installed_versions_require_no_network_or_install(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest = Path(directory) / "package.json"
            manifest.write_text(json.dumps(self.package()), encoding="utf-8")
            with (
                patch.object(tools.sys, "platform", "linux"),
                patch.object(tools.platform, "machine", return_value="x86_64"),
                patch.object(tools.os, "geteuid", return_value=0),
                patch.object(
                    tools,
                    "installed_version",
                    side_effect=lambda name: {
                        "node": "24.16.0",
                        "pnpm": "11.11.0",
                    }[name],
                ),
                patch.object(tools.urllib.request, "urlopen") as download,
                patch.object(tools.subprocess, "run") as run,
            ):
                tools.install_toolchain(manifest)
                download.assert_not_called()
                run.assert_not_called()

    def test_container_setup_reads_manifest_in_container_and_preserves_endpoint(
        self,
    ):
        with patch.object(tools.subprocess, "run") as run:
            tools.configure_toolchain(
                ["docker", "--context", "selected"], {}, "test-container"
            )
        commands = [call.args[0] for call in run.call_args_list]
        self.assertTrue(
            all(
                command[:3] == ["docker", "--context", "selected"]
                for command in commands
            )
        )
        self.assertIn("--install-toolchain", commands[-1])
        self.assertEqual(
            commands[-1][commands[-1].index("--workdir") + 1],
            "/workspace/src/brave",
        )
        self.assertFalse(
            any(
                "package.json" in word
                for command in commands
                for word in command
            )
        )

    def test_local_installer_cli_needs_no_docker_or_credentials(self):
        with (
            patch.object(
                tools.sys,
                "argv",
                ["create_container.py", "--install-toolchain"],
            ),
            patch.object(tools, "install_toolchain") as install,
            patch.object(tools, "docker_connection") as docker,
        ):
            self.assertEqual(tools.main(), 0)
            install.assert_called_once_with(Path("package.json"))
            docker.assert_not_called()


if __name__ == "__main__":
    unittest.main()
