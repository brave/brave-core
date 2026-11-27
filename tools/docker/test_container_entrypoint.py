# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

"""Verify bootstrap startup and cached Windows SDK mount recovery."""

import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest


class ContainerEntrypointTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(tempfile.mkdtemp()).resolve()
        self.addCleanup(shutil.rmtree, self.root)
        dockerfile = (
            Path(__file__).with_name("Dockerfile").read_text(encoding="utf-8")
        )
        self.script = (
            re.search(
                r"<<'ENTRYPOINT'[^\n]*\n(.*?)\nENTRYPOINT", dockerfile, re.S
            )
            .group(1)
            .replace("/workspace/src", str(self.root))
        )
        self.sdk = self.root / "third_party/depot_tools/win_toolchain/vs_files"
        self.backing = Path(str(self.sdk) + ".ciopfs")
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.calls = self.root / "mount-calls"
        mountpoint = self.bin / "mountpoint"
        mountpoint.write_text(
            '#!/bin/sh\ntest -e "$2/.mounted"\n', encoding="utf-8"
        )
        mountpoint.chmod(0o755)
        self.environment = {
            **os.environ,
            "PATH": str(self.bin) + os.pathsep + os.environ.get("PATH", ""),
        }
        for name in ("RBE_tls_client_auth_cert", "RBE_tls_client_auth_key"):
            self.environment.pop(name, None)

    def cached_sdk(self, failure=0):
        self.backing.mkdir(parents=True)
        (self.backing / "header.h").write_text("cached SDK\n", encoding="utf-8")
        ciopfs = self.root / "build/ciopfs"
        ciopfs.parent.mkdir()
        ciopfs.write_text(
            f"#!{sys.executable}\n"
            "import json, pathlib, shutil, sys\n"
            f"pathlib.Path({str(self.calls)!r}).write_text("
            "json.dumps(sys.argv[1:]), encoding='utf-8')\n"
            f"if {failure}: sys.exit({failure})\n"
            "source, target = map(pathlib.Path, sys.argv[3:])\n"
            "shutil.copyfile(source / 'header.h', target / 'header.h')\n"
            "(target / '.mounted').touch()\n",
            encoding="utf-8",
        )
        ciopfs.chmod(0o755)

    def start(self, *command):
        return subprocess.run(
            ["bash", "-c", self.script, "entrypoint", *command],
            cwd=self.root,
            env=self.environment,
            capture_output=True,
            text=True,
            check=False,
        )

    def test_initial_start_needs_no_sdk_mount_helper_or_credentials(self):
        command = [
            sys.executable,
            "-c",
            "import json, os, sys; "
            "print(json.dumps([os.getcwd(), sys.argv[1:]])); sys.exit(7)",
            "argument with spaces",
            "--target_os",
            "win",
        ]
        result = self.start(*command)
        self.assertEqual(result.returncode, 7, result.stderr)
        self.assertEqual(
            json.loads(result.stdout),
            [str(self.root), ["argument with spaces", "--target_os", "win"]],
        )
        self.assertFalse(self.sdk.exists())
        self.assertFalse(self.calls.exists())

    def test_cached_sdk_is_recovered_before_requested_command(self):
        self.cached_sdk()
        result = self.start("cat", str(self.sdk / "header.h"))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "cached SDK\n")
        self.assertEqual(
            json.loads(self.calls.read_text(encoding="utf-8")),
            ["-o", "use_ino", str(self.backing), str(self.sdk)],
        )

    def test_existing_mount_is_preserved_without_invoking_helper(self):
        self.cached_sdk(failure=19)
        self.sdk.mkdir()
        (self.sdk / ".mounted").touch()
        result = self.start("true")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(self.calls.exists())

    def test_mount_failure_prevents_requested_command(self):
        self.cached_sdk(failure=19)
        marker = self.root / "command-ran"
        result = self.start("touch", str(marker))
        self.assertEqual(result.returncode, 19, result.stderr)
        self.assertFalse(marker.exists())


if __name__ == "__main__":
    unittest.main()
