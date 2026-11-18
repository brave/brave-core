#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

"""Create and start a Brave development container from an existing image.

Select the exact image tag printed by build_brave_image.py; no image is built
by this command. Docker uses DOCKER_HOST and DOCKER_CONTEXT as usual.

Readable mTLS credentials default to ~/.reclient/engflow.crt and
~/.reclient/engflow.key; BRAVE_RBE_CERT and BRAVE_RBE_KEY override those paths.
They are mounted read-only at the image's configured credential destinations.
The GitHub SSH identity is resolved from ~/.ssh/config unless --ssh-key is
supplied. That key and ~/.ssh/known_hosts are mounted read-only for the
container's lifetime. known_hosts must contain verified GitHub host keys, and
the key must authenticate without an agent or passphrase prompt. Source files
must be accessible to the Docker daemon and readable by the image's builder.

The container enables GitHub SSH authentication and SSH commit signing using
that identity. Supply a Git user name and email before committing, and register
the selected public key with GitHub as a signing key for verified signatures.
The container requires /dev/fuse, SYS_ADMIN, and an unconfined AppArmor profile
for the Windows SDK hook. It keeps the image's checkout unchanged and does not
fetch refs, synchronize sources, or run builds. Failed setup retains the
container for diagnosis; no containers or build outputs are removed.

Example:
  python3 create_container.py --image brave-linux-build:v1.101.39 --name dev
"""

import argparse
import getpass
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from build_brave_image import (
    IMAGE_FORMAT,
    credential_mounts,
    credentials,
    docker_connection,
    inspect,
)

SSH_KEY = "/home/builder/.ssh/github_key"
SSH_COMMAND = (
    f"ssh -F /dev/null -i {SSH_KEY} -o IdentitiesOnly=yes "
    "-o IdentityAgent=none "
    "-o AddKeysToAgent=no -o BatchMode=yes -o StrictHostKeyChecking=yes"
)


def resolve_ssh_key(override=None):
    """Return the supplied key or github.com's configured SSH identity."""
    if override is not None:
        return Path(override).expanduser().resolve()
    config = Path.home() / ".ssh/config"
    if not config.is_file():
        raise RuntimeError(
            "No ~/.ssh/config for github.com; supply --ssh-key PATH"
        )
    effective = subprocess.check_output(
        [
            "ssh",
            "-G",
            "-F",
            str(config),
            "-o",
            "IdentityFile=none",
            "git@github.com",
        ],
        text=True,
    )
    settings = [
        line.split(None, 1) for line in effective.splitlines() if " " in line
    ]
    hostname = next(
        (value for key, value in settings if key == "hostname"), "github.com"
    )
    tokens = {
        "%d": str(Path.home()),
        "%h": hostname,
        "%n": "github.com",
        "%r": "git",
        "%u": getpass.getuser(),
        "%%": "%",
    }
    paths = set()
    for key, value in settings:
        if key != "identityfile" or value == "none":
            continue
        value = re.sub(r"%[%dhnru]", lambda match: tokens[match.group()], value)
        path = Path(value).expanduser().resolve()
        if path.is_file() and os.access(path, os.R_OK):
            paths.add(path)
    if len(paths) != 1:
        raise RuntimeError(
            "Cannot determine one readable github.com SSH key "
            "from ~/.ssh/config; "
            "supply --ssh-key PATH"
        )
    return paths.pop()


def host_ssh_files(ssh_key):
    """Resolve the selected key and the host's trusted hosts."""
    root = Path.home() / ".ssh"
    files = {
        SSH_KEY: Path(ssh_key).expanduser().resolve(),
        "/home/builder/.ssh/known_hosts": (root / "known_hosts").resolve(),
    }
    for path in files.values():
        if not path.is_file() or not os.access(path, os.R_OK):
            raise RuntimeError(
                f"Host SSH credential file is missing or unreadable: {path}"
            )
    return files


def verify_ssh_mounts(docker, identifier, ssh_key, *, env=None):
    """Reject attempts to replace a container's lifetime SSH key mount."""
    files = host_ssh_files(ssh_key)
    mounts = json.loads(
        subprocess.check_output(
            [
                *docker,
                "container",
                "inspect",
                "--format",
                "{{json .Mounts}}",
                identifier,
            ],
            text=True,
            env=env,
        )
    )
    mounted = {mount["Destination"]: mount for mount in mounts}
    for destination, source in files.items():
        mount = mounted.get(destination, {})
        if mount.get("Source") != str(source) or mount.get("RW", True):
            raise RuntimeError(
                "Container has different SSH mounts; its existing key persists "
                "for its lifetime. Ask the user what to do"
            )


def configure_ssh(docker, identifier, ssh_key=None, *, env=None):
    """Configure Git authentication and signing using the container's key."""
    if ssh_key is not None:
        verify_ssh_mounts(docker, identifier, ssh_key, env=env)
    script = '''set -euo pipefail
test -r "$1"
git config --global core.sshCommand "$2"
git config --global gpg.format ssh
git config --global user.signingkey "$1"
git config --global commit.gpgsign true
git config --global url."ssh://git@github.com".insteadOf https://github.com
'''
    subprocess.run(
        [
            *docker,
            "exec",
            "--workdir",
            "/workspace/src/brave",
            identifier,
            "bash",
            "-c",
            script,
            "configure-brave-ssh",
            SSH_KEY,
            SSH_COMMAND,
        ],
        check=True,
        env=env,
    )


def start_container(docker, env, name, *, running=False, paused=False):
    """Start a stopped container, retaining running or paused containers."""
    if paused:
        raise RuntimeError(f"Container {name} is paused")
    if not running:
        subprocess.run([docker, "start", name], check=True, env=env)


def create_container(docker, env, name, image_info, ssh_key=None, labels=()):
    """Create and start a container with credential mounts and SSH setup.

    The caller selects an image with the current checkout format. Labels are
    passed through without interpreting checkout refs or changing sources.
    """
    certificate, key = credentials()
    ssh_key = resolve_ssh_key(ssh_key)
    files = host_ssh_files(ssh_key)
    ssh_mounts = [
        argument
        for destination, source in files.items()
        for argument in (
            "--mount",
            f"type=bind,source={source},target={destination},readonly",
        )
    ]
    label_args = [
        argument for label in labels for argument in ("--label", label)
    ]
    subprocess.run(
        [
            docker,
            "create",
            "--name",
            name,
            "--init",
            "--platform=linux/amd64",
            "--shm-size=4g",
            "--device",
            "/dev/fuse",
            "--cap-add",
            "SYS_ADMIN",
            "--security-opt",
            "apparmor=unconfined",
            *label_args,
            "--label",
            "io.brave.checkout.credentials=host-ssh-files",
            *ssh_mounts,
            "--env",
            f"GIT_SSH_COMMAND={SSH_COMMAND}",
            *credential_mounts(image_info, certificate, key),
            image_info["Id"],
        ],
        check=True,
        env=env,
    )
    start_container(docker, env, name)
    configure_ssh([docker], name, ssh_key, env=env)


def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument(
        "--image", required=True, help="Existing image tag or ID"
    )
    parser.add_argument(
        "--name", required=True, help="Name for the new persistent container"
    )
    parser.add_argument(
        "--ssh-key",
        type=Path,
        metavar="PATH",
        help="GitHub SSH key; otherwise use ~/.ssh/config",
    )
    parser.add_argument(
        "--docker",
        metavar="EXECUTABLE",
        help="Docker executable (default: docker from PATH)",
    )
    return parser.parse_args(argv)


def main():
    args = parse_args()
    try:
        docker, env = docker_connection(args.docker)
        if inspect(docker, env, "container", args.name) is not None:
            raise RuntimeError(
                f"Container {args.name} already exists; select a new name"
            )
        image_info = inspect(docker, env, "image", args.image)
        if (
            image_info is None
            or (image_info["Config"].get("Labels") or {}).get(
                "io.brave.checkout.format"
            )
            != IMAGE_FORMAT
        ):
            raise RuntimeError(
                f"Checkout image {args.image} is unavailable or incompatible"
            )
        create_container(docker, env, args.name, image_info, args.ssh_key)
    except subprocess.CalledProcessError as error:
        print(f"Error: {error}", file=sys.stderr)
        return (
            error.returncode
            if error.returncode >= 0
            else 128 - error.returncode
        )
    except RuntimeError as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    print(f"Started container: {args.name}", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
