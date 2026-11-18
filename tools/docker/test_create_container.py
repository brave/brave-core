# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

"""Verify SSH configuration discovery and lifetime credential mounts."""

import contextlib
import io
import json
import os
import shutil
from pathlib import Path
import subprocess
import tempfile
import time
import unittest
from unittest.mock import patch

import create_container as container_tools


@contextlib.contextmanager
def running_agent(socket):
    """Keep an isolated test agent running until its signing check finishes."""
    with subprocess.Popen(
        ["ssh-agent", "-D", "-a", socket],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    ) as process:
        try:
            for _ in range(100):
                if Path(socket).exists():
                    break
                time.sleep(0.01)
            yield
        finally:
            process.terminate()
            process.wait(timeout=5)


class SSHCredentialTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(tempfile.mkdtemp()).resolve()
        self.addCleanup(shutil.rmtree, self.root)
        self.ssh = self.root / ".ssh"
        self.ssh.mkdir()
        self.key = self.ssh / "github-key"
        self.key.write_text("test fixture")
        self.enterContext(
            patch.object(container_tools.Path, "home", return_value=self.root)
        )

    def test_config_includes_and_home_tokens_are_resolved(self):
        include = self.ssh / "included.conf"
        include.write_text(
            "Host github.com\n  IdentityFile %d/.ssh/github-key\n"
        )
        (self.ssh / "config").write_text(f"Include {include}\n")
        self.assertEqual(container_tools.resolve_ssh_key(), self.key)

    def test_missing_or_ambiguous_config_requires_override(self):
        with self.assertRaisesRegex(RuntimeError, "--ssh-key"):
            container_tools.resolve_ssh_key()
        other = self.ssh / "other-key"
        other.write_text("test fixture")
        (self.ssh / "config").write_text(
            f"Host github.com\n  IdentityFile {self.key}\n"
            f"  IdentityFile {other}\n"
        )
        with self.assertRaisesRegex(RuntimeError, "--ssh-key"):
            container_tools.resolve_ssh_key()

    def test_default_ssh_identities_are_not_guessed(self):
        (self.ssh / "config").write_text("Host github.com\n  User git\n")
        (self.ssh / "id_rsa").write_text("unconfigured key")
        with self.assertRaisesRegex(RuntimeError, "--ssh-key"):
            container_tools.resolve_ssh_key()

    def test_existing_container_cannot_change_key_mount(self):
        known_hosts = self.ssh / "known_hosts"
        known_hosts.write_text("test fixture")
        mounts = [
            {
                "Destination": container_tools.SSH_KEY,
                "Source": str(self.key),
                "RW": False,
            },
            {
                "Destination": "/home/builder/.ssh/known_hosts",
                "Source": str(known_hosts),
                "RW": False,
            },
        ]
        with patch.object(
            container_tools.subprocess,
            "check_output",
            return_value=json.dumps(mounts),
        ):
            container_tools.verify_ssh_mounts(
                ["docker"], "test-container", self.key
            )
            other = self.ssh / "other-key"
            other.write_text("test fixture")
            with self.assertRaisesRegex(
                RuntimeError, "persists for its lifetime"
            ):
                container_tools.verify_ssh_mounts(
                    ["docker"], "test-container", other
                )

    def test_agent_prefers_configured_public_key_and_strips_comments(self):
        (self.ssh / "config").write_text(
            f"Host github.com\n  IdentityFile {self.key}\n"
        )
        Path(str(self.key) + ".pub").write_text(
            "ssh-ed25519 BBBB configured\n", encoding="utf-8"
        )
        self.assertEqual(
            container_tools.agent_identity(
                "ssh-ed25519 AAAA first\nssh-ed25519 BBBB second\n"
            ),
            "ssh-ed25519 BBBB",
        )
        self.assertIsNone(
            container_tools.agent_identity("ssh-ed25519 AAAA first\n")
        )

    def test_agent_without_config_uses_first_public_identity(self):
        self.assertEqual(
            container_tools.agent_identity(
                "ssh-ed25519 AAAA first\nssh-ed25519 BBBB second\n"
            ),
            "ssh-ed25519 AAAA",
        )
        self.assertIsNone(
            container_tools.agent_identity("The agent has no identities.\n")
        )

    def check_signing(self, agent=False):
        signing_key = self.root / "signing-key"
        subprocess.run(
            [
                "ssh-keygen",
                "-q",
                "-t",
                "ed25519",
                "-N",
                "",
                "-f",
                str(signing_key),
            ],
            check=True,
        )
        environment = dict(
            os.environ,
            GIT_CONFIG_GLOBAL=str(self.root / "gitconfig"),
            GIT_CONFIG_NOSYSTEM="1",
        )
        if agent:
            socket = str(self.root / "agent.sock")
            self.enterContext(running_agent(socket))
            environment["SSH_AUTH_SOCK"] = socket
            subprocess.run(
                ["ssh-add", str(signing_key)],
                env=environment,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=True,
            )
            signing_key.rename(self.root / "unmounted-private-key")
            self.enterContext(
                patch.object(container_tools, "SSH_AGENT", socket)
            )
            self.enterContext(
                patch.object(
                    container_tools,
                    "SSH_AGENT_KEY",
                    str(self.root / "agent.pub"),
                )
            )
        real_run = subprocess.run

        def docker_run(command, **kwargs):
            return real_run(
                command[command.index("bash") :],
                check=kwargs.pop("check", True),
                **kwargs,
            )

        with (
            patch.object(container_tools, "SSH_KEY", str(signing_key)),
            patch.object(
                container_tools.subprocess, "run", side_effect=docker_run
            ),
        ):
            container_tools.configure_ssh(
                ["test-docker"],
                "test-container",
                env=environment,
                agent_key=signing_key.with_suffix(".pub").read_text()
                if agent
                else None,
            )
        repo = self.root / "repository"
        subprocess.run(
            ["git", "init", "-q", str(repo)], check=True, env=environment
        )
        subprocess.run(
            [
                "git",
                "-C",
                str(repo),
                "-c",
                "user.name=Signing Test",
                "-c",
                "user.email=signing-test",
                "commit",
                "--allow-empty",
                "-qm",
                "Signing check",
            ],
            check=True,
            env=environment,
        )
        signers = self.root / "allowed-signers"
        signers.write_text(
            "signing-test " + signing_key.with_suffix(".pub").read_text()
        )
        result = subprocess.run(
            [
                "git",
                "-C",
                str(repo),
                "-c",
                f"gpg.ssh.allowedSignersFile={signers}",
                "verify-commit",
                "HEAD",
            ],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=environment,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Good", result.stderr)

    def test_direct_key_enables_verifiable_commit_signing(self):
        self.check_signing()

    def test_forwarded_agent_signs_without_mounted_private_key(self):
        self.check_signing(agent=True)


class AgentDetectionTests(unittest.TestCase):
    def test_bridge_is_probed_on_mac_using_selected_daemon_and_removed(self):
        environment = {
            "DOCKER_HOST": "ssh://docker-host",
            "SSH_AUTH_SOCK": "/host/agent",
        }
        with (
            patch.object(container_tools.sys, "platform", "darwin"),
            patch.object(
                container_tools,
                "agent_identity",
                return_value="ssh-ed25519 AAAA",
            ),
            patch.object(
                container_tools.subprocess,
                "run",
                side_effect=[
                    subprocess.CompletedProcess([], 0, "probe-id\n", ""),
                    subprocess.CompletedProcess(
                        [], 0, "ssh-ed25519 AAAA comment\n", ""
                    ),
                    subprocess.CompletedProcess([], 0),
                ],
            ) as run,
        ):
            result = container_tools.find_ssh_agent(
                "docker", environment, {"Id": "image"}
            )
        self.assertEqual(
            result, ("/run/host-services/ssh-auth.sock", "ssh-ed25519 AAAA")
        )
        commands = [call.args[0] for call in run.call_args_list]
        self.assertIn(
            "type=bind,source=/run/host-services/ssh-auth.sock,"
            "target=/run/brave-ssh-agent.sock,readonly",
            commands[0],
        )
        self.assertEqual(commands[-1], ["docker", "rm", "--force", "probe-id"])
        self.assertTrue(
            all(
                call.kwargs["env"] == environment for call in run.call_args_list
            )
        )

    def test_unavailable_bridge_tries_environment_socket(self):
        with (
            patch.object(container_tools.sys, "platform", "darwin"),
            patch.object(
                container_tools,
                "agent_identity",
                return_value="ssh-ed25519 AAAA",
            ),
            patch.object(
                container_tools.subprocess,
                "run",
                side_effect=[
                    subprocess.CompletedProcess([], 1, "", "missing socket"),
                    subprocess.CompletedProcess([], 0, "probe-id\n", ""),
                    subprocess.CompletedProcess(
                        [], 0, "ssh-ed25519 AAAA\n", ""
                    ),
                    subprocess.CompletedProcess([], 0),
                ],
            ),
        ):
            self.assertEqual(
                container_tools.find_ssh_agent(
                    "docker", {"SSH_AUTH_SOCK": "/host/agent"}, {"Id": "image"}
                ),
                ("/host/agent", "ssh-ed25519 AAAA"),
            )

    def test_empty_agent_and_timeouts_remove_probes_before_fallback(self):
        for outcome in (
            subprocess.CompletedProcess(
                [], 1, "", "The agent has no identities."
            ),
            subprocess.TimeoutExpired(["docker", "start"], 15),
        ):
            with (
                self.subTest(outcome=outcome),
                patch.object(container_tools.sys, "platform", "linux"),
                contextlib.redirect_stderr(io.StringIO()),
                patch.object(
                    container_tools.subprocess,
                    "run",
                    side_effect=[
                        subprocess.CompletedProcess([], 0, "probe-id\n", ""),
                        outcome,
                        subprocess.CompletedProcess([], 0),
                        subprocess.CompletedProcess(
                            [], 1, "", "missing socket"
                        ),
                    ],
                ) as run,
            ):
                self.assertIsNone(
                    container_tools.find_ssh_agent(
                        "docker",
                        {"SSH_AUTH_SOCK": "/host/agent"},
                        {"Id": "image"},
                    )
                )
            self.assertEqual(
                run.call_args_list[2].args[0],
                ["docker", "rm", "--force", "probe-id"],
            )


class ContainerCreationTests(unittest.TestCase):
    def setUp(self):
        self.image = {
            "Id": "image-id",
            "Config": {
                "Env": [
                    "RBE_tls_client_auth_cert=/run/cert",
                    "RBE_tls_client_auth_key=/run/key",
                ]
            },
        }
        self.environment = {"DOCKER_HOST": "ssh://docker-host"}
        self.key = Path("/fixture/github-key")
        self.hosts = Path("/fixture/known_hosts")
        self.enterContext(
            patch.object(
                container_tools,
                "credentials",
                return_value=("/fixture/cert", "/fixture/key"),
            )
        )
        self.resolve = self.enterContext(
            patch.object(
                container_tools, "resolve_ssh_key", return_value=self.key
            )
        )
        self.files = self.enterContext(
            patch.object(
                container_tools,
                "host_ssh_files",
                return_value={
                    container_tools.SSH_KEY: self.key,
                    "/home/builder/.ssh/known_hosts": self.hosts,
                },
            )
        )
        self.enterContext(
            patch.object(
                container_tools, "host_known_hosts", return_value=self.hosts
            )
        )
        self.agent = self.enterContext(
            patch.object(container_tools, "find_ssh_agent", return_value=None)
        )
        self.configure = self.enterContext(
            patch.object(container_tools, "configure_ssh")
        )
        self.run = self.enterContext(
            patch.object(
                container_tools.subprocess,
                "run",
                return_value=subprocess.CompletedProcess([], 0),
            )
        )

    def test_creation_mounts_credentials_readonly_and_preserves_endpoint(self):
        container_tools.create_container(
            "docker",
            self.environment,
            "development",
            self.image,
            self.key,
            labels=("custom.label=value",),
        )
        create, start = [call.args[0] for call in self.run.call_args_list]
        mounts = [
            create[index + 1]
            for index, argument in enumerate(create)
            if argument == "--mount"
        ]
        self.assertCountEqual(
            mounts,
            [
                "type=bind,source=/fixture/github-key,"
                "target=/home/builder/.ssh/github_key,readonly",
                "type=bind,source=/fixture/known_hosts,"
                "target=/home/builder/.ssh/known_hosts,readonly",
                "type=bind,source=/fixture/cert,target=/run/cert,readonly",
                "type=bind,source=/fixture/key,target=/run/key,readonly",
            ],
        )
        self.assertIn("/dev/fuse", create)
        self.assertIn("SYS_ADMIN", create)
        self.assertIn("apparmor=unconfined", create)
        self.assertIn("custom.label=value", create)
        self.assertIn("GIT_SSH_COMMAND=" + container_tools.SSH_COMMAND, create)
        self.assertEqual(create[-1], "image-id")
        self.assertEqual(start, ["docker", "start", "development"])
        self.assertTrue(
            all(
                call.kwargs["env"] == self.environment
                for call in self.run.call_args_list
            )
        )
        self.configure.assert_called_once_with(
            ["docker"], "development", self.key, env=self.environment
        )
        self.agent.assert_not_called()

    def test_agent_mode_mounts_socket_without_private_key(self):
        self.agent.return_value = ("/fixture/agent.sock", "ssh-ed25519 AAAA")
        container_tools.create_container(
            "docker", self.environment, "development", self.image
        )
        create = self.run.call_args_list[0].args[0]
        self.assertIn(
            "type=bind,source=/fixture/agent.sock,"
            "target=/run/brave-ssh-agent.sock,readonly",
            create,
        )
        self.assertIn("SSH_AUTH_SOCK=/run/brave-ssh-agent.sock", create)
        self.assertIn("io.brave.checkout.credentials=host-ssh-agent", create)
        self.resolve.assert_not_called()
        self.files.assert_not_called()
        self.configure.assert_called_once_with(
            ["docker"],
            "development",
            env=self.environment,
            agent_key="ssh-ed25519 AAAA\n",
        )

    def test_no_agent_falls_back_to_configured_key(self):
        container_tools.create_container(
            "docker", self.environment, "development", self.image
        )
        self.agent.assert_called_once_with(
            "docker", self.environment, self.image
        )
        self.resolve.assert_called_once_with(None)
        self.configure.assert_called_once_with(
            ["docker"], "development", self.key, env=self.environment
        )

    def test_missing_ssh_file_stops_before_creation(self):
        self.files.side_effect = RuntimeError("missing SSH file")
        with self.assertRaisesRegex(RuntimeError, "missing SSH file"):
            container_tools.create_container(
                "docker", self.environment, "development", self.image, self.key
            )
        self.run.assert_not_called()
        self.configure.assert_not_called()

    def test_creation_failure_does_not_start_or_remove_container(self):
        self.run.side_effect = subprocess.CalledProcessError(
            7, ["docker", "create"]
        )
        with self.assertRaises(subprocess.CalledProcessError):
            container_tools.create_container(
                "docker", self.environment, "development", self.image, self.key
            )
        self.run.assert_called_once()
        self.configure.assert_not_called()

    def test_start_failure_retains_created_container_without_ssh_setup(self):
        self.run.side_effect = [
            subprocess.CompletedProcess([], 0),
            subprocess.CalledProcessError(9, ["docker", "start"]),
        ]
        with self.assertRaises(subprocess.CalledProcessError):
            container_tools.create_container(
                "docker", self.environment, "development", self.image, self.key
            )
        self.assertEqual(
            [call.args[0][1] for call in self.run.call_args_list],
            ["create", "start"],
        )
        self.configure.assert_not_called()

    def test_running_and_paused_containers_are_not_restarted(self):
        container_tools.start_container(
            "docker", self.environment, "development", running=True
        )
        with self.assertRaisesRegex(RuntimeError, "paused"):
            container_tools.start_container(
                "docker", self.environment, "development", paused=True
            )
        self.run.assert_not_called()


class ContainerCommandLineTests(unittest.TestCase):
    def setUp(self):
        self.create = None

    def run_main(self, objects):
        self.enterContext(
            patch.object(
                container_tools,
                "docker_connection",
                return_value=("docker", {}),
            )
        )
        self.enterContext(
            patch.object(container_tools, "inspect", side_effect=objects)
        )
        self.create = self.enterContext(
            patch.object(container_tools, "create_container")
        )
        self.enterContext(contextlib.redirect_stderr(io.StringIO()))
        self.enterContext(contextlib.redirect_stdout(io.StringIO()))
        self.enterContext(
            patch.object(
                container_tools.sys,
                "argv",
                [
                    "create_container.py",
                    "--image",
                    "explicit-image",
                    "--name",
                    "development",
                ],
            )
        )
        return container_tools.main()

    def test_existing_container_is_retained(self):
        self.assertEqual(self.run_main([{"Id": "existing"}]), 1)
        self.create.assert_not_called()

    def test_missing_image_does_not_create_or_build(self):
        self.assertEqual(self.run_main([None, None]), 1)
        self.create.assert_not_called()

    def test_explicit_image_is_used_without_checkout_selection(self):
        image = {
            "Id": "image-id",
            "Config": {
                "Labels": {
                    "io.brave.checkout.format": container_tools.IMAGE_FORMAT,
                }
            },
        }
        self.assertEqual(self.run_main([None, image]), 0)
        self.create.assert_called_once_with(
            "docker", {}, "development", image, None
        )

    def test_help_requires_no_docker_or_credentials(self):
        with (
            patch.object(container_tools, "docker_connection") as connect,
            contextlib.redirect_stdout(io.StringIO()),
            self.assertRaises(SystemExit) as result,
        ):
            container_tools.parse_args(["--help"])
        self.assertEqual(result.exception.code, 0)
        connect.assert_not_called()


if __name__ == "__main__":
    unittest.main()
