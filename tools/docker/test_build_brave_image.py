# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

"""Verify the portable image builder and the Dockerfile's runtime command."""

import contextlib
import io
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import build_brave_image as builder


class DockerSelectionTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(self.enterContext(tempfile.TemporaryDirectory()))
        self.path_directory = self.root / "path"
        self.path_directory.mkdir()
        self.enterContext(
            patch.dict(os.environ, {"PATH": str(self.path_directory)})
        )

    def executable(self, path, status=0):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(f"#!/bin/sh\nexit {status}\n")
        path.chmod(0o755)
        return str(path)

    def test_explicit_executable_is_preferred_over_docker_on_path(self):
        self.executable(self.path_directory / "docker", status=7)
        explicit = self.executable(
            self.root / "custom directory" / "docker-command"
        )
        selected, _ = builder.docker_connection(explicit)
        self.assertEqual(selected, explicit)

    def test_omitted_option_uses_docker_from_path(self):
        expected = self.executable(self.path_directory / "docker")
        selected, _ = builder.docker_connection()
        self.assertEqual(selected, expected)

    def test_invalid_explicit_executable_does_not_fall_back(self):
        self.executable(self.path_directory / "docker")
        with self.assertRaises(RuntimeError):
            builder.docker_binary(str(self.root / "missing"))

    def test_missing_path_command_reports_an_error(self):
        with self.assertRaisesRegex(
            RuntimeError, "not found or is not executable"
        ):
            builder.docker_binary()

    def test_cli_option_reaches_the_image_builder(self):
        with (
            patch.object(
                builder.sys,
                "argv",
                ["build_brave_image.py", "master", "--docker", "chosen-docker"],
            ),
            patch.object(builder, "_is_agent", return_value=False),
            patch.object(builder, "build_image") as build,
        ):
            self.assertEqual(builder.main(), 0)
        build.assert_called_once_with(
            "master", None, docker_executable="chosen-docker"
        )


class ImageBuildTests(unittest.TestCase):
    def image(self):
        return {
            "Config": {
                "Env": [
                    "RBE_tls_client_auth_cert=/custom/certificate",
                    "RBE_tls_client_auth_key=/custom/key",
                ]
            }
        }

    def test_builder_runs_with_only_its_documented_support_files(self):
        with tempfile.TemporaryDirectory() as directory:
            for name in (
                "build_brave_image.py",
                "create_container.py",
                "Dockerfile",
                ".dockerignore",
            ):
                shutil.copyfile(builder.SUPPORT / name, Path(directory) / name)
            result = subprocess.run(
                [
                    sys.executable,
                    "-I",
                    str(Path(directory) / "build_brave_image.py"),
                    "--help",
                ],
                text=True,
                capture_output=True,
                check=True,
            )
        self.assertIn("--git-cache-path", result.stdout)

    def test_credential_mounts_follow_image_configuration(self):
        self.assertEqual(
            builder.credential_mounts(self.image(), "/host/cert", "/host/key"),
            [
                "--mount",
                "type=bind,source=/host/cert,target=/custom/certificate,readonly",
                "--mount",
                "type=bind,source=/host/key,target=/custom/key,readonly",
            ],
        )
        with self.assertRaises(RuntimeError):
            builder.credential_mounts(
                {"Config": {"Env": []}}, "/host/cert", "/host/key"
            )

    def run_build(self, failure=False):
        container = {"Id": "image-container", "State": {"Running": True}}
        commands = []

        def run(command, **kwargs):
            commands.append(command)
            if failure and "/usr/local/bin/brave-build" in command:
                raise subprocess.CalledProcessError(7, command)
            return subprocess.CompletedProcess(command, 0)

        with (
            patch.object(
                builder, "resolve_reference", return_value=("branch", "commit")
            ),
            patch.object(
                builder, "credentials", return_value=("/host/cert", "/host/key")
            ),
            patch.object(
                builder,
                "docker_connection",
                return_value=("docker", {"DOCKER_HOST": "tcp://builder:2375"}),
            ),
            patch.object(
                builder, "inspect", side_effect=[None, self.image(), container]
            ),
            patch.object(builder, "verify_checkout"),
            patch.object(
                builder.subprocess, "check_output", return_value="default\n"
            ),
            patch.object(builder.subprocess, "run", side_effect=run),
            contextlib.redirect_stdout(io.StringIO()),
            contextlib.redirect_stderr(io.StringIO()),
        ):
            if failure:
                with self.assertRaises(subprocess.CalledProcessError):
                    builder.build_image("master")
            else:
                builder.build_image("master")
        return commands

    def test_image_build_installs_dependencies_once_before_compiling_and_publishing(
        self,
    ):
        commands = self.run_build()
        select = next(
            command for command in commands if "prepare-image" in command
        )
        toolchain = next(
            command for command in commands if "--install-toolchain" in command
        )
        initialize = next(
            command
            for command in commands
            if "pnpm" in command and "init" in command
        )
        self.assertLess(commands.index(select), commands.index(toolchain))
        self.assertLess(commands.index(toolchain), commands.index(initialize))
        installs = [
            command
            for command in commands
            if "build/install-build-deps.sh" in command
        ]
        self.assertEqual(len(installs), 1)
        self.assertIn("--android", installs[0])
        builds = [
            command
            for command in commands
            if "/usr/local/bin/brave-build" in command
        ]
        self.assertEqual(len(builds), 4)
        for command in builds:
            build_command = command.index("/usr/local/bin/brave-build")
            self.assertEqual(command[build_command + 1], "Component")
        platforms = {
            (
                command[command.index("--target_os") + 1],
                command[command.index("--target_arch") + 1],
            )
            for command in builds
        }
        self.assertEqual(
            platforms,
            {
                ("mac", "x64"),
                ("win", "x64"),
                ("linux", "x64"),
                ("android", "arm64"),
            },
        )
        self.assertLess(commands.index(installs[0]), commands.index(builds[0]))
        commit = next(
            command
            for command in commands
            if "commit" in command and "--change" in command
        )
        self.assertGreater(commands.index(commit), commands.index(builds[-1]))
        self.assertTrue(
            all(
                command[:3] == ["docker", "--host", "tcp://builder:2375"]
                for command in commands
            )
        )

    def test_failed_compile_retains_container_and_does_not_publish(self):
        commands = self.run_build(failure=True)
        self.assertFalse(
            any(
                "--change" in command or "rm" in command or "stop" in command
                for command in commands
            )
        )

    def test_retained_old_container_is_rejected_before_changes(self):
        old = {"Config": {"Labels": {"io.brave.checkout.format": "5"}}}
        with (
            patch.object(
                builder, "resolve_reference", return_value=("branch", "commit")
            ),
            patch.object(
                builder, "credentials", return_value=("/host/cert", "/host/key")
            ),
            patch.object(
                builder, "docker_connection", return_value=("docker", {})
            ),
            patch.object(builder, "inspect", return_value=old),
            patch.object(
                builder.subprocess, "check_output", return_value="default\n"
            ),
            patch.object(builder.subprocess, "run") as run,
        ):
            with self.assertRaisesRegex(RuntimeError, "retained for migration"):
                builder.build_image("master")
        run.assert_not_called()


class ImageLoggingTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(self.enterContext(tempfile.TemporaryDirectory()))
        self.portable = self.root / "portable"
        self.portable.mkdir()
        for name in (
            "build_brave_image.py",
            "create_container.py",
            "Dockerfile",
            ".dockerignore",
        ):
            shutil.copyfile(builder.SUPPORT / name, self.portable / name)
        self.cwd = self.root / "working-directory"
        self.cwd.mkdir()
        self.environment = dict(os.environ)
        for name in (
            "CODEX_THREAD_ID",
            "CODEX_SESSION_ID",
            "CODEX_CI",
            "CLAUDECODE",
            "CLAUDE_CODE_REMOTE",
            "CURSOR_AGENT",
            "GEMINI_CLI",
        ):
            self.environment.pop(name, None)

    def run_build(self, agent_value, status=0):
        harness = '''import importlib.util, subprocess, sys
spec = importlib.util.spec_from_file_location("image_builder", sys.argv[1])
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)
status = int(sys.argv[2])
def build_image(*args, **kwargs):
    print("python stdout")
    print("python stderr", file=sys.stderr)
    subprocess.run([sys.executable, "-c",
                    "import os; os.write(1, b'child stdout' + bytes([10])); os.write(2, b'child stderr' + bytes([10]))"], check=True)
    if status:
        raise subprocess.CalledProcessError(status, ["fake-build"])
builder.build_image = build_image
sys.argv = ["build_brave_image.py", "master"]
sys.exit(builder.main())
'''
        return subprocess.run(
            [
                sys.executable,
                "-I",
                "-c",
                harness,
                str(self.portable / "build_brave_image.py"),
                str(status),
            ],
            cwd=self.cwd,
            env={**self.environment, "CODEX_CI": agent_value},
            text=True,
            capture_output=True,
        )

    def test_agent_logs_python_and_child_output_in_cwd_and_preserves_status(
        self,
    ):
        for status, expected in ((0, 0), (7, 7), (-15, 143)):
            with self.subTest(status=status):
                result = self.run_build("1", status)
                self.assertEqual(result.returncode, expected)
                lines = result.stdout.splitlines()
                self.assertEqual(len(lines), 2)
                self.assertTrue(lines[0].startswith("Log: "))
                self.assertEqual(lines[1], f"Exit code: {expected}")
                self.assertEqual(result.stderr, "")
                path = Path(lines[0][len("Log: ") :])
                self.assertEqual(path.parent.resolve(), self.cwd.resolve())
                output = path.read_text()
                for marker in (
                    "python stdout",
                    "python stderr",
                    "child stdout",
                    "child stderr",
                    f"Exit code: {expected}",
                ):
                    self.assertIn(marker, output)
                if status:
                    self.assertIn("Error:", output)

    def test_non_agent_streams_output_without_creating_a_log(self):
        for agent_value in ("", "0", "false", "no"):
            with self.subTest(agent_value=agent_value):
                result = self.run_build(agent_value, 7)
                self.assertEqual(result.returncode, 7)
                self.assertIn("python stdout", result.stdout)
                self.assertIn("child stdout", result.stdout)
                self.assertIn("python stderr", result.stderr)
                self.assertIn("child stderr", result.stderr)
                self.assertIn("Error:", result.stderr)
                self.assertEqual(list(self.cwd.iterdir()), [])

    def test_help_does_not_create_an_agent_log(self):
        result = subprocess.run(
            [
                sys.executable,
                "-I",
                str(self.portable / "build_brave_image.py"),
                "--help",
            ],
            cwd=self.cwd,
            env={**self.environment, "CODEX_CI": "1"},
            text=True,
            capture_output=True,
            check=True,
        )
        for topic in (
            "Requirements:",
            "Builds and image tags:",
            "Credentials and Docker selection:",
            "Failure recovery and output:",
            "Container layout and further builds:",
            "Examples:",
            "BRAVE_RBE_CERT",
            "BRAVE_RBE_KEY",
            "DOCKER_HOST",
            "DOCKER_CONTEXT",
            "--git-cache-path",
        ):
            self.assertIn(topic, result.stdout)
        self.assertEqual(list(self.cwd.iterdir()), [])


class ImageCommandTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(self.enterContext(tempfile.TemporaryDirectory()))
        self.dockerfile = (builder.SUPPORT / "Dockerfile").read_text()
        environment = re.search(
            r"^ENV LANG=.*?(?=\n\n)", self.dockerfile, re.M | re.S
        ).group()
        self.environment = dict(
            item.split("=", 1)
            for item in environment[4:].replace("\\\n", "").split()
        )
        for setting in ("RBE_tls_client_auth_cert", "RBE_tls_client_auth_key"):
            credential = self.root / setting
            credential.touch()
            self.environment[setting] = str(credential)
        pnpm = self.root / "pnpm"
        pnpm.write_text(
            "#!/usr/bin/env python3\nimport json, os, sys\n"
            "print(json.dumps({'args': sys.argv[1:], 'cwd': os.getcwd(), "
            "'helper': os.environ['SISO_CREDENTIAL_HELPER']}))\n"
            "sys.exit(int(os.environ.get('TEST_BUILD_EXIT', '0')))\n"
        )
        pnpm.chmod(0o755)
        self.environment = {
            **os.environ,
            **self.environment,
            "PATH": str(self.root) + os.pathsep + os.environ.get("PATH", ""),
        }

    def command(self, *arguments, **overrides):
        script = re.search(
            r"<<'BUILD'[^\n]*\n(.*?)\nBUILD", self.dockerfile, re.S
        ).group(1)
        return subprocess.run(
            ["bash", "-c", script, "brave-build", *arguments],
            cwd=self.root,
            env={**self.environment, **overrides},
            text=True,
            capture_output=True,
        )

    def test_build_command_preserves_arguments_directory_credentials_and_exit_status(
        self,
    ):
        arguments = ["Static", "--target_os", "win", "-C", "output with spaces"]
        result = self.command(*arguments, TEST_BUILD_EXIT="7")
        self.assertEqual(result.returncode, 7)
        self.assertEqual(
            json.loads(result.stdout),
            {
                "args": ["run", "build", *arguments],
                "cwd": str(self.root.resolve()),
                "helper": "mTLS",
            },
        )

    def test_missing_credential_prevents_compilation(self):
        result = self.command(
            RBE_tls_client_auth_key=str(self.root / "missing-key")
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, "")

    def test_dotenv_uses_remote_defaults_and_only_enables_supplied_git_cache(
        self,
    ):
        script = re.search(
            r"RUN <<'CONFIG'\n(.*?)\nCONFIG", self.dockerfile, re.S
        ).group(1)
        subprocess.run(
            ["sh", "-c", script],
            cwd=self.root,
            env=self.environment,
            check=True,
        )
        config = dict(
            line.split("=", 1)
            for line in (self.root / ".env").read_text().splitlines()
        )
        for setting in (
            "RBE_service",
            "RBE_tls_client_auth_cert",
            "RBE_tls_client_auth_key",
        ):
            self.assertEqual(config[setting.lower()], self.environment[setting])
        self.assertEqual(config["use_remoteexec"], "true")
        self.assertNotIn("cache_dir", config)
        self.assertNotIn("git_cache_path", config)
        cached_stage = self.dockerfile.split(
            "FROM checkout AS checkout-cached\n", 1
        )[1]
        script = next(
            line[4:]
            for line in cached_stage.splitlines()
            if line.startswith("RUN ")
        )
        subprocess.run(
            ["sh", "-c", script],
            cwd=self.root,
            env=self.environment,
            check=True,
        )
        cached_config = dict(
            line.split("=", 1)
            for line in (self.root / ".env").read_text().splitlines()
        )
        self.assertEqual(
            cached_config,
            {**config, "git_cache_path": "/home/builder/.cache/brave/git"},
        )


if __name__ == "__main__":
    unittest.main()
