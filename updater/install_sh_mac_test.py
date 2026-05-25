#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
This script tests Brave's customizations to chrome/updater/mac/.install.sh.
You can run it with `python3 install_sh_mac_test.py`.
"""

from os import chmod, makedirs, mkdir, stat
from os.path import dirname, exists, join, realpath
from stat import S_IXUSR
from subprocess import run, DEVNULL, Popen, PIPE, STDOUT
from tempfile import TemporaryDirectory

import plistlib
import re
import shlex
import sys
import unittest

SRC_ROOT = dirname(dirname(dirname(realpath(__file__))))
INSTALL_SH = join(SRC_ROOT, "chrome", "updater", "mac", ".install.sh")

COMMAND_PROMPT = "command exit code: "
COMMAND_WRAPPER = f"""
#!/bin/bash

printf -v args '%q ' "$@"
echo "{COMMAND_PROMPT}$(basename "$0") ${{args}}"
read -r code
exit "$code"
"""

PRODUCT_NAME = "Brave Browser"
CURRENT_VERSION = "1.0.0.0"
UPDATE_VERSION = "2.0.0.0"


@unittest.skipUnless(sys.platform == "darwin", "requires macOS")
class InstallShPatchTest(unittest.TestCase):

    def setUp(self):
        self.temp_dir = TemporaryDirectory()
        self.dmg_dir = self._prepare_dmg_dir()
        self.install_sh, self.bin_dir = self._prepare_install_sh()

    def tearDown(self):
        self.temp_dir.cleanup()

    def test_nonstandard_app_dir_name(self):
        app_dir = join(self.temp_dir.name, "User Renamed Brave.app")
        self._make_app(app_dir, CURRENT_VERSION)
        self._run_install_sh(app_dir)
        self._check_app(app_dir, UPDATE_VERSION)

    def test_nonroot_omits_perms_and_link_and_dir_times(self):
        # See Chromium CL 5866112.
        app_dir = join(self.temp_dir.name, f"{PRODUCT_NAME}.app")
        self._make_app(app_dir, CURRENT_VERSION)
        rsync_args = []
        def rsync(args):
            rsync_args.extend(args)
            return system_rsync(args)
        self._run_install_sh(app_dir, commands={'rsync': rsync})
        for arg in ('--ignore-times', '--links', '--no-perms',
                    '--executability', '--chmod=u=rwX,go=rX'):
            self.assertIn(arg, rsync_args)
        for arg in ('--perms', '--times'):
            self.assertNotIn(arg, rsync_args)

    def test_root_has_perms_and_link_and_dir_times(self):
        app_dir = join(self.temp_dir.name, f"{PRODUCT_NAME}.app")
        self._make_app(app_dir, CURRENT_VERSION)
        rsync_args = []
        def rsync(args):
            rsync_args.extend(args)
            return system_rsync(args)
        self._run_install_sh(app_dir, is_root=True, commands={'rsync': rsync})
        for arg in ('--ignore-times', '--links', '--perms', '--times'):
            self.assertIn(arg, rsync_args)
        for arg in ('--no-perms', '--executability', '--chmod=u=rwX,go=rX'):
            self.assertNotIn(arg, rsync_args)

    def _prepare_dmg_dir(self):
        dmg_dir = join(self.temp_dir.name, "dmg")
        mkdir(dmg_dir)
        self._make_app(join(dmg_dir, f"{PRODUCT_NAME}.app"), UPDATE_VERSION)
        return dmg_dir

    def _prepare_install_sh(self):
        with open(INSTALL_SH, "r") as f:
            source = f.read()
        patched, count = re.subn(r"^UPDATE_VERSION=\s*$",
                                 f'UPDATE_VERSION="{UPDATE_VERSION}"',
                                 source,
                                 count=1,
                                 flags=re.MULTILINE)
        self.assertEqual(1, count)
        bin_dir = join(self.temp_dir.name, "bin")
        mkdir(bin_dir)
        # Prepend bin/ to PATH so tests can override commands like rsync.
        patched, count = re.subn(r'^export PATH="',
                                 f'export PATH="{bin_dir}:',
                                 patched,
                                 count=1,
                                 flags=re.MULTILINE)
        self.assertEqual(1, count)
        install_sh_path = join(self.temp_dir.name, "install.sh")
        with open(install_sh_path, "w") as f:
            f.write(patched)
        chmod(install_sh_path, stat(install_sh_path).st_mode | S_IXUSR)
        return install_sh_path, bin_dir

    def _make_app(self, bundle_path, version):
        """Create the minimum .app bundle that .install.sh's checks accept."""
        contents = join(bundle_path, "Contents")
        framework_dir = join(contents, "Frameworks",
                             f"{PRODUCT_NAME} Framework.framework")
        makedirs(join(framework_dir, "Versions", version, "Resources"))
        with open(join(contents, "Info.plist"), "wb") as f:
            plistlib.dump(
                {
                    "CFBundleDisplayName": PRODUCT_NAME,
                    "CFBundleShortVersionString": version,
                    "CFBundleExecutable": PRODUCT_NAME,
                },
                f,
            )

    def _check_app(self, bundle_path, version):
        """Assert a .app bundle has the structure _make_app would create."""
        with open(join(bundle_path, "Contents", "Info.plist"), "rb") as f:
            plist = plistlib.load(f)
        self.assertEqual(version, plist["CFBundleShortVersionString"])
        versioned_dir = join(
            bundle_path,
            "Contents",
            "Frameworks",
            f"{PRODUCT_NAME} Framework.framework",
            "Versions",
            version,
        )
        self.assertTrue(exists(versioned_dir), msg=versioned_dir)

    def _run_install_sh(self, installed_app_dir, is_root=False, commands=None):
        commands = commands or {}
        for name in commands:
            wrapper_path = join(self.bin_dir, name)
            with open(wrapper_path, "w") as f:
                f.write(COMMAND_WRAPPER)
            chmod(wrapper_path, stat(wrapper_path).st_mode | S_IXUSR)
        env = {}
        if is_root:
            env["EUID"] = "0"
        proc = Popen(
            [
                self.install_sh, self.dmg_dir, installed_app_dir,
                CURRENT_VERSION
            ],
            stdin=PIPE, stdout=PIPE, stderr=STDOUT, text=True, bufsize=1,
            env=env
        )
        output_lines = []
        try:
            for line in proc.stdout:
                output_lines.append(line)
                if line.startswith(COMMAND_PROMPT):
                    name, *args = shlex.split(line[len(COMMAND_PROMPT):])
                    exit_code = commands[name](args) or 0
                    proc.stdin.write(f"{exit_code}\n")
                    proc.stdin.flush()
            proc.wait(timeout=30)
        finally:
            # Reap the subprocess if wait() timed out or the loop raised.
            if proc.poll() is None:
                proc.kill()
                proc.wait()
        output = "".join(output_lines)
        self.assertEqual(
            0, proc.returncode,
            f"Command {self.install_sh} failed with return code "
            f"{proc.returncode}.\n\nOutput:\n{output}")


def system_rsync(args):
    return run(
        ["/usr/bin/rsync"] + args, stdout=DEVNULL, stderr=DEVNULL
    ).returncode


if __name__ == "__main__":
    unittest.main()
