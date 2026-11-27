# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

"""Verify real process ownership and crash release without Docker builds."""

import contextlib
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import build_brave_image as builder


HARNESS = '''import sys
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0, sys.argv[1])
import build_brave_image as builder
with patch.object(builder.Path, "home", return_value=Path(sys.argv[2])), \\
     patch.object(builder.subprocess, "check_output", return_value=sys.argv[3]):
    try:
        with builder.image_lock(["unused-docker"], {}, sys.argv[4]):
            print("locked", flush=True)
            if sys.argv[5] == "hold":
                sys.stdin.read()
    except RuntimeError as error:
        print(str(error), file=sys.stderr)
        sys.exit(2)
'''


class ImageLockTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(tempfile.mkdtemp()).resolve()
        self.addCleanup(shutil.rmtree, self.root)
        self.enterContext(
            patch.object(builder.Path, "home", return_value=self.root)
        )
        self.enterContext(
            patch.object(
                builder.subprocess, "check_output", return_value="daemon"
            )
        )

    def command(self, *, daemon="daemon", name="image-container", mode="try"):
        return [
            sys.executable,
            "-I",
            "-c",
            HARNESS,
            str(builder.SUPPORT),
            str(self.root),
            daemon,
            name,
            mode,
        ]

    @contextlib.contextmanager
    def owner(self):
        with subprocess.Popen(
            self.command(mode="hold"),
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        ) as process:
            try:
                self.assertEqual(process.stdout.readline().strip(), "locked")
                yield process
            finally:
                if process.poll() is None:
                    process.kill()
                process.communicate(timeout=5)

    def attempt(self, **options):
        return subprocess.run(
            self.command(**options),
            capture_output=True,
            text=True,
            check=False,
            timeout=5,
        )

    def test_second_process_is_rejected_until_owner_exits(self):
        with self.owner() as owner:
            rejected = self.attempt()
            self.assertEqual(rejected.returncode, 2)
            self.assertIn("already running", rejected.stderr)
            owner.communicate(input="", timeout=5)
            self.assertEqual(owner.returncode, 0)
            self.assertEqual(self.attempt().returncode, 0)

    def test_sigkill_releases_lock_without_deleting_its_file(self):
        with self.owner() as owner:
            lock = next((self.root / ".cache/brave/image-locks").glob("*.lock"))
            inode = lock.stat().st_ino
            owner.kill()
            owner.wait(timeout=5)
            self.assertTrue(lock.exists())
            self.assertEqual(self.attempt().returncode, 0)
            self.assertEqual(lock.stat().st_ino, inode)

    def test_exception_releases_ownership(self):
        with self.assertRaisesRegex(ValueError, "failure"):
            with builder.image_lock(["docker"], {}, "image-container"):
                raise ValueError("failure")
        self.assertEqual(self.attempt().returncode, 0)

    def test_distinct_daemons_or_containers_do_not_block_each_other(self):
        with self.owner():
            self.assertEqual(self.attempt(daemon="other-daemon").returncode, 0)
            self.assertEqual(self.attempt(name="other-container").returncode, 0)

    def test_context_aliases_for_one_daemon_share_ownership(self):
        with builder.image_lock(
            ["docker", "--context", "first-alias"], {}, "image-container"
        ):
            with self.assertRaisesRegex(RuntimeError, "already running"):
                with builder.image_lock(
                    ["docker", "--context", "second-alias"],
                    {},
                    "image-container",
                ):
                    self.fail("Context alias bypassed ownership")

    def test_missing_daemon_identity_does_not_allow_unlocked_operation(self):
        with patch.object(builder.subprocess, "check_output", return_value=""):
            with self.assertRaisesRegex(RuntimeError, "lock identity"):
                with builder.image_lock(["docker"], {}, "image-container"):
                    self.fail("Missing daemon identity bypassed ownership")
        self.assertFalse((self.root / ".cache/brave/image-locks").exists())

    def test_contending_builder_stops_before_reference_or_container_work(self):
        name = (
            "brave-image-" + builder.hashlib.sha256(b"master").hexdigest()[:16]
        )
        with (
            builder.image_lock(["docker"], {}, name),
            patch.object(
                builder, "docker_connection", return_value=("docker", {})
            ),
            patch.object(builder, "_build_image") as build,
        ):
            with self.assertRaisesRegex(RuntimeError, "already running"):
                builder.build_image("master")
        build.assert_not_called()


if __name__ == "__main__":
    unittest.main()
