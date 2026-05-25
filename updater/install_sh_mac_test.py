#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
This script tests Brave's customizations to chrome/updater/mac/.install.sh.
You can run it with `python3 install_sh_mac_test.py`.
"""

from os import chmod, close, fdopen, makedirs, mkdir, pipe, stat
from os.path import dirname, exists, join, realpath
from stat import S_IXUSR
from subprocess import run, DEVNULL, Popen, PIPE, STDOUT
from tempfile import TemporaryDirectory
from threading import Thread

import plistlib
import re
import shlex
import sys
import unittest

SRC_ROOT = dirname(dirname(dirname(realpath(__file__))))
INSTALL_SH = join(SRC_ROOT, "chrome", "updater", "mac", ".install.sh")

# We make it possible for the Python test to mock system executables. For
# example:
#     def mkdir(args):
#         return 1, "Permission denied"
#     self._run_install_sh(..., commands={'mkdir': mkdir})
# This runs .install.sh with a modified PATH that includes a wrapper for `mkdir`
# that returns 1 and prints "Permission denied" to stderr.
# The way this works is that `mkdir` on PATH has the contents given by
# COMMAND_WRAPPER (CW) below. CW writes the command name and arguments to a
# dedicated file descriptor (PROMPT_FD). The Python implementation reads from
# this file descriptor, invokes the associated Python function and writes the
# exit code and stderr to CW's stdin. CW then prints the given stderr to stderr
# and exits with the given code.
COMMAND_WRAPPER = """
#!/bin/bash

printf -v args '%q ' "$@"
echo "$(basename "$0") ${args}" >&"${PROMPT_FD}"
read -r exit_code
read -r stderr
if [[ -n "${stderr}" ]]; then
  echo "${stderr}" >&2
fi
exit "$exit_code"
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

    def test_versioned_rsync_retry_succeeds(self):
        """Initial versioned rsync fails, mkdir succeeds, retry succeeds."""
        app_dir = join(self.temp_dir.name, f"{PRODUCT_NAME}.app")
        self._make_app(app_dir, CURRENT_VERSION)
        calls = []

        def rsync(args):
            calls.append(args)
            if len(calls) == 1:
                return 1, ""
            return system_rsync(args)

        self._run_install_sh(app_dir,
                             commands={'rsync': rsync},
                             expected_exit_code=76)
        self.assertEqual(2, len(calls))

    def test_versioned_rsync_into_parent_succeeds(self):
        """Initial rsync and retry both fail; rsync into parent succeeds."""
        app_dir = join(self.temp_dir.name, f"{PRODUCT_NAME}.app")
        self._make_app(app_dir, CURRENT_VERSION)
        calls = []

        def rsync(args):
            calls.append(args)
            if len(calls) <= 2:
                return 1, ""
            return system_rsync(args)

        self._run_install_sh(app_dir,
                             commands={'rsync': rsync},
                             expected_exit_code=77)
        self.assertEqual(3, len(calls))
        # The third invocation rsyncs into installed_versions_dir, with no
        # trailing slash on the source path.
        self.assertFalse(calls[2][-2].endswith("/"), calls[2])

    def test_versioned_rsync_clean_slate_parent_succeeds(self):
        """First three rsync attempts fail; clean-slate parent-rsync succeeds."""
        app_dir = join(self.temp_dir.name, f"{PRODUCT_NAME}.app")
        self._make_app(app_dir, CURRENT_VERSION)
        calls = []

        def rsync(args):
            calls.append(args)
            if len(calls) <= 3:
                return 1, ""
            return system_rsync(args)

        self._run_install_sh(app_dir,
                             commands={'rsync': rsync},
                             expected_exit_code=78)
        self.assertEqual(4, len(calls))

    def test_versioned_rsync_all_attempts_fail(self):
        app_dir = join(self.temp_dir.name, f"{PRODUCT_NAME}.app")
        self._make_app(app_dir, CURRENT_VERSION)
        self._run_install_sh(app_dir,
                             commands={'rsync': lambda args: (1, '')},
                             expected_exit_code=79)

    def test_mkdir_failure_classified_by_stderr(self):
        """
        When mkdir of new_versioned_dir fails, install.sh classifies the
        stderr into a distinct exit code in [70, 75].
        """
        cases = [
            (70, "Read-only file system"),
            (71, "No space left on device"),
            (71, "Disc quota exceeded"),
            (72, "Operation not permitted"),
            (73, "Permission denied"),
            (74, "File exists"),
            (74, "Not a directory"),
            (75, "Some other error"),
        ]
        for i, (expected_exit_code, stderr_msg) in enumerate(cases):
            with self.subTest(stderr=stderr_msg):
                app_dir = join(self.temp_dir.name, f"App-{i}.app")
                self._make_app(app_dir, CURRENT_VERSION)

                def mock_mkdir(args, msg=stderr_msg):
                    # The earlier `mkdir -p installed_versions_dir` runs
                    # against a path that already exists; only the recovery
                    # mkdir targets a path containing UPDATE_VERSION.
                    if UPDATE_VERSION in args[-1]:
                        return 1, f"mkdir: {args[-1]}: {msg}"
                    return 0, ""

                self._run_install_sh(app_dir,
                                     commands={
                                         'rsync': lambda args: (1, ""),
                                         'mkdir': mock_mkdir
                                     },
                                     expected_exit_code=expected_exit_code)

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

    def _run_install_sh(self,
                        installed_app_dir,
                        is_root=False,
                        commands=None,
                        expected_exit_code=0):
        commands = commands or {}
        for name in commands:
            wrapper_path = join(self.bin_dir, name)
            with open(wrapper_path, "w") as f:
                f.write(COMMAND_WRAPPER)
            chmod(wrapper_path, stat(wrapper_path).st_mode | S_IXUSR)
        prompt_r, prompt_w = pipe()
        env = {"PROMPT_FD": str(prompt_w)}
        if is_root:
            env["EUID"] = "0"
        proc = Popen([
            self.install_sh, self.dmg_dir, installed_app_dir, CURRENT_VERSION
        ],
                     stdin=PIPE,
                     stdout=PIPE,
                     stderr=STDOUT,
                     text=True,
                     bufsize=1,
                     env=env,
                     pass_fds=(prompt_w, ))
        # The subprocess inherited its own copy of prompt_w via pass_fds. A
        # pipe only reaches EOF once *every* writer has closed its end, so we
        # must drop our copy here; otherwise the read loop below would block
        # forever even after the subprocess and its children exited.
        close(prompt_w)
        output_lines = []

        def drain():
            for line in proc.stdout:
                output_lines.append(line)

        drain_thread = Thread(target=drain)
        drain_thread.start()
        try:
            with fdopen(prompt_r, "r") as prompts:
                for line in prompts:
                    name, *args = shlex.split(line)
                    exit_code, stderr = commands[name](args)
                    proc.stdin.write(f"{exit_code}\n{stderr}\n")
                    proc.stdin.flush()
            proc.wait(timeout=30)
        finally:
            # Reap the subprocess if wait() timed out or the loop raised.
            if proc.poll() is None:
                proc.kill()
                proc.wait()
            drain_thread.join(timeout=5)
        output = "".join(output_lines)
        self.assertEqual(
            expected_exit_code, proc.returncode,
            f"Command {self.install_sh} exited with code {proc.returncode} "
            f"instead of {expected_exit_code}.\n\nOutput:\n{output}")


def system_rsync(args):
    cp = run(["/usr/bin/rsync"] + args,
             stdout=DEVNULL,
             stderr=PIPE,
             text=True,
             check=False)
    return cp.returncode, cp.stderr


if __name__ == "__main__":
    unittest.main()
