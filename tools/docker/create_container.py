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
SSH-agent forwarding is preferred: probe SSH_AUTH_SOCK and the Docker Desktop /
OrbStack bridge at /run/host-services/ssh-auth.sock using the selected daemon.
On macOS the bridge is tried first. Load your key into the host agent before
running this command; passphrase and approval prompts stay on the host.
The configured GitHub identity's public key is preferred when available;
otherwise the agent's first identity is used for authentication and signing.
If no usable agent is available, resolve the key from ~/.ssh/config and mount
it read-only. --ssh-key PATH explicitly selects this direct-key mode, which
requires a key usable without an agent or passphrase prompt.
Both modes mount ~/.ssh/known_hosts read-only; it must contain verified GitHub
host keys. Source files must be accessible to the Docker daemon and readable
by the image's builder. The chosen SSH mounts persist for the container's
lifetime. Probe containers are removed immediately after checking the agent.

The container enables GitHub SSH authentication and SSH commit signing using
that identity. Supply a Git user name and email before committing, and register
the selected public key with GitHub as a signing key for verified signatures.
The container requires /dev/fuse, SYS_ADMIN, and an unconfined AppArmor profile
for the Windows SDK hook. It keeps the image's checkout unchanged and does not
fetch refs, synchronize sources, or run builds. Failed setup retains the
development container for diagnosis; development containers and build outputs
are never removed.

Node and pnpm are installed from the checked-out package.json. For supported
stable version ranges, setup selects the minimum version. devEngines takes
precedence over legacy engines; an exact pnpm packageManager pin takes
precedence over its engine range. Missing or unsupported requirements fail
instead of falling back to hardcoded versions. After selecting another ref,
run --setup-toolchain --name NAME before init, sync, or build. This does not
select refs or synchronize sources. --install-toolchain runs the same setup
locally inside the Linux container as root, without Docker or credentials.

Example:
  python3 create_container.py --image brave-linux-build:v1.101.39 --name dev
"""

import argparse
import getpass
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import uuid

from build_brave_image import (
    IMAGE_FORMAT,
    configure_toolchain,
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
SSH_AGENT = "/run/brave-ssh-agent.sock"
SSH_AGENT_KEY = "/home/builder/.brave-github.pub"
AGENT_SSH_COMMAND = (
    f"ssh -F /dev/null -i {SSH_AGENT_KEY} -o IdentitiesOnly=yes "
    f"-o IdentityAgent={SSH_AGENT} "
    "-o AddKeysToAgent=no -o BatchMode=yes -o StrictHostKeyChecking=yes"
)


def minimum_version(requirement):
    """Resolve stable exact/comparator/caret/tilde ranges without Node."""
    candidates = []
    for alternative in requirement.split("||"):
        pattern = r"([<>=~^]*)\s*(\d+\.\d+\.\d+)"
        tokens = re.findall(pattern, alternative)
        if not tokens or re.sub(pattern, "", alternative).strip():
            raise RuntimeError(
                f"Unsupported stable version range: {requirement!r}"
            )
        lower = (0, 0, 0)
        upper = None
        for operator, value in tokens:
            version = tuple(map(int, value.split(".")))
            if operator in ("", "=", ">=", ">", "^", "~"):
                floor = version
                if operator == ">":
                    floor = (*version[:2], version[2] + 1)
                lower = max(lower, floor)
            if operator in ("", "=", "<=", "<", "^", "~"):
                ceiling = (*version[:2], version[2] + 1)
                if operator == "<":
                    ceiling = version
                elif operator == "~":
                    ceiling = (version[0], version[1] + 1, 0)
                elif operator == "^":
                    index = next(
                        (i for i, part in enumerate(version) if part), 2
                    )
                    ceiling = (
                        *version[:index],
                        version[index] + 1,
                        *([0] * (2 - index)),
                    )
                upper = ceiling if upper is None else min(upper, ceiling)
            if operator not in ("", "=", ">=", ">", "<=", "<", "^", "~"):
                raise RuntimeError(
                    f"Unsupported stable version range: {requirement!r}"
                )
        if upper is None or lower < upper:
            candidates.append(lower)
    if not candidates:
        raise RuntimeError(f"Empty stable version range: {requirement!r}")
    return ".".join(map(str, min(candidates)))


def toolchain_versions(package):
    """Read this checkout's Node requirement and pnpm pin or requirement."""
    if not isinstance(package, dict):
        raise RuntimeError("package.json must contain an object")
    development = package.get("devEngines", {})
    legacy = package.get("engines", {})
    if not isinstance(development, dict) or not isinstance(legacy, dict):
        raise RuntimeError("package.json engine settings must contain objects")
    versions = {}
    for name, setting in (("node", "runtime"), ("pnpm", "packageManager")):
        engine = development.get(setting)
        if engine is not None and (
            not isinstance(engine, dict) or engine.get("name") != name
        ):
            raise RuntimeError(f"Unsupported package.json devEngines.{setting}")
        requirement = engine.get("version") if engine else legacy.get(name)
        if name == "pnpm" and package.get("packageManager"):
            pin = re.fullmatch(
                r"pnpm@(\d+\.\d+\.\d+)(?:\+sha\d+\.[a-fA-F0-9]+)?",
                package["packageManager"],
            )
            if not pin:
                raise RuntimeError(
                    "package.json packageManager must pin a stable pnpm version"
                )
            requirement = pin.group(1)
            constraint = engine.get("version") if engine else legacy.get(name)
            if constraint:
                if not isinstance(constraint, str):
                    raise RuntimeError(
                        "package.json pnpm version must be a string"
                    )
                minimum_version(
                    " || ".join(
                        part + " " + requirement
                        for part in constraint.split("||")
                    )
                )
        if not isinstance(requirement, str) or not requirement:
            raise RuntimeError(
                f"package.json has no {name} version requirement"
            )
        versions[name] = minimum_version(requirement)
    return versions


def installed_version(command):
    """Return the installed version, or None if the command is unavailable."""
    try:
        return (
            subprocess.check_output(
                [command, "--version"],
                cwd="/",
                text=True,
                stderr=subprocess.DEVNULL,
            )
            .strip()
            .removeprefix("v")
        )
    except (FileNotFoundError, subprocess.CalledProcessError):
        return None


def install_toolchain(package_path):
    """Install the checkout's selected versions in a Linux x64 container."""
    versions = toolchain_versions(
        json.loads(package_path.read_text(encoding="utf-8"))
    )
    if (
        sys.platform != "linux"
        or platform.machine() not in ("x86_64", "amd64")
        or os.geteuid() != 0
    ):
        raise RuntimeError(
            "--install-toolchain requires root inside the Linux x64 container"
        )
    node = versions["node"]
    if installed_version("node") != node:
        destination = Path("/opt/brave/node") / node
        if not destination.exists():
            destination.parent.mkdir(parents=True, exist_ok=True)
            archive = f"node-v{node}-linux-x64.tar.xz"
            base = f"https://nodejs.org/dist/v{node}/"
            with tempfile.TemporaryDirectory(
                dir=destination.parent
            ) as temporary:
                root = Path(temporary)
                with urllib.request.urlopen(
                    base + "SHASUMS256.txt", timeout=60
                ) as response:
                    sums = dict(
                        line.split()[::-1]
                        for line in response.read().decode().splitlines()
                    )
                with (
                    urllib.request.urlopen(
                        base + archive, timeout=60
                    ) as response,
                    (root / archive).open("wb") as output,
                ):
                    shutil.copyfileobj(response, output)
                with (root / archive).open("rb") as downloaded:
                    digest = hashlib.file_digest(
                        downloaded, "sha256"
                    ).hexdigest()
                if digest != sums.get(archive):
                    raise RuntimeError(
                        "Node download checksum does not match SHASUMS256.txt"
                    )
                extracted = root / "node"
                extracted.mkdir()
                subprocess.run(
                    [
                        "tar",
                        "-xJf",
                        str(root / archive),
                        "--strip-components=1",
                        "-C",
                        str(extracted),
                    ],
                    check=True,
                )
                extracted.rename(destination)
        for name in ("node", "npm", "npx"):
            link = Path("/usr/local/bin") / name
            replacement = link.with_name(name + ".brave-new")
            replacement.symlink_to(destination / "bin" / name)
            replacement.replace(link)
    if installed_version("node") != node:
        raise RuntimeError(
            f"Installed Node does not match package.json's selected {node}"
        )
    if installed_version("pnpm") != versions["pnpm"]:
        subprocess.run(
            [
                "npm",
                "install",
                "--global",
                "--prefix",
                "/usr/local",
                "--no-audit",
                "--no-fund",
                "pnpm@" + versions["pnpm"],
            ],
            cwd="/",
            check=True,
        )
    if installed_version("pnpm") != versions["pnpm"]:
        raise RuntimeError("Installed pnpm does not match package.json")
    print(f"Toolchain: Node {node}, pnpm {versions['pnpm']}", flush=True)


def agent_identity(output):
    """Select a configured public identity, or the agent's first key."""
    keys = [
        " ".join(parts[:2])
        for line in output.splitlines()
        if len(parts := line.split()) >= 2
        and parts[0].startswith(("ssh-", "ecdsa-", "sk-"))
    ]
    if not keys:
        return None
    try:
        identity = resolve_ssh_key()
    except RuntimeError:
        return keys[0]
    public = (
        identity if identity.suffix == ".pub" else Path(str(identity) + ".pub")
    )
    if not public.is_file() or not os.access(public, os.R_OK):
        return keys[0]
    configured = " ".join(public.read_text(encoding="utf-8").split()[:2])
    return configured if configured in keys else None


def find_ssh_agent(docker, env, image_info):
    """Probe agent sockets as the image's user and remove each probe."""
    bridge = "/run/host-services/ssh-auth.sock"
    candidates = [env.get("SSH_AUTH_SOCK"), bridge]
    if sys.platform == "darwin":
        candidates.reverse()
    for source in dict.fromkeys(path for path in candidates if path):
        result = subprocess.run(
            [
                docker,
                "create",
                "--name",
                "brave-ssh-probe-" + uuid.uuid4().hex,
                "--platform=linux/amd64",
                "--network=none",
                "--entrypoint",
                "ssh-add",
                "--mount",
                f"type=bind,source={source},target={SSH_AGENT},readonly",
                "--env",
                f"SSH_AUTH_SOCK={SSH_AGENT}",
                image_info["Id"],
                "-L",
            ],
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )
        if result.returncode:
            continue
        identifier = result.stdout.strip()
        try:
            result = subprocess.run(
                [docker, "start", "--attach", identifier],
                env=env,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                timeout=15,
                check=False,
            )
            if result.returncode == 0:
                identity = agent_identity(result.stdout)
                if identity:
                    return source, identity
        except subprocess.TimeoutExpired:
            print(
                "SSH agent probe timed out; trying another socket",
                file=sys.stderr,
            )
        finally:
            subprocess.run(
                [docker, "rm", "--force", identifier],
                env=env,
                stdout=subprocess.DEVNULL,
                check=True,
            )
    return None


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


def host_known_hosts():
    """Return the host's readable trusted-hosts file."""
    path = (Path.home() / ".ssh/known_hosts").resolve()
    if not path.is_file() or not os.access(path, os.R_OK):
        raise RuntimeError(
            f"Host SSH credential file is missing or unreadable: {path}"
        )
    return path


def host_ssh_files(ssh_key):
    """Resolve the selected key and the host's trusted hosts."""
    files = {
        SSH_KEY: Path(ssh_key).expanduser().resolve(),
        "/home/builder/.ssh/known_hosts": host_known_hosts(),
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


def configure_ssh(
    docker, identifier, ssh_key=None, *, env=None, agent_key=None
):
    """Configure authentication and signing without changing SSH mounts."""
    if ssh_key is not None:
        verify_ssh_mounts(docker, identifier, ssh_key, env=env)
    script = '''set -euo pipefail
if [ "${SSH_AUTH_SOCK:-}" = "$3" ]; then
  if [ ! -s "$4" ]; then
    cat > "$4"
  fi
  ssh-add -T "$4"
  git config --global core.sshCommand "$5"
  git config --global user.signingkey "$4"
else
  test -r "$1"
  git config --global core.sshCommand "$2"
  git config --global user.signingkey "$1"
fi
git config --global gpg.format ssh
git config --global commit.gpgsign true
git config --global url."ssh://git@github.com".insteadOf https://github.com
'''
    subprocess.run(
        [
            *docker,
            "exec",
            "--interactive",
            "--workdir",
            "/workspace/src/brave",
            identifier,
            "bash",
            "-c",
            script,
            "configure-brave-ssh",
            SSH_KEY,
            SSH_COMMAND,
            SSH_AGENT,
            SSH_AGENT_KEY,
            AGENT_SSH_COMMAND,
        ],
        check=True,
        env=env,
        input=agent_key or "",
        text=True,
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
    agent = find_ssh_agent(docker, env, image_info) if ssh_key is None else None
    if agent:
        source, public_key = agent
        hosts = host_known_hosts()
        files = {SSH_AGENT: source, "/home/builder/.ssh/known_hosts": hosts}
        ssh_environment = ["--env", f"SSH_AUTH_SOCK={SSH_AGENT}"]
        ssh_command = AGENT_SSH_COMMAND
        mode = "host-ssh-agent"
    else:
        ssh_key = resolve_ssh_key(ssh_key)
        files = host_ssh_files(ssh_key)
        ssh_environment = []
        ssh_command = SSH_COMMAND
        mode = "host-ssh-files"
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
            f"io.brave.checkout.credentials={mode}",
            *ssh_mounts,
            *ssh_environment,
            "--env",
            f"GIT_SSH_COMMAND={ssh_command}",
            *credential_mounts(image_info, certificate, key),
            image_info["Id"],
        ],
        check=True,
        env=env,
    )
    start_container(docker, env, name)
    configure_toolchain([docker], env, name)
    if agent:
        configure_ssh([docker], name, env=env, agent_key=public_key + "\n")
    else:
        configure_ssh([docker], name, ssh_key, env=env)
    print(
        "SSH mode: " + ("forwarded agent" if agent else "direct key mounts"),
        flush=True,
    )


def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--image", help="Existing image tag or ID")
    mode.add_argument(
        "--setup-toolchain",
        action="store_true",
        help="Install Node/pnpm for an existing container's current checkout",
    )
    mode.add_argument(
        "--install-toolchain",
        action="store_true",
        help="Install Node/pnpm locally inside the Linux container as root",
    )
    parser.add_argument(
        "--name", help="Name of the new or existing persistent container"
    )
    parser.add_argument(
        "--ssh-key",
        type=Path,
        metavar="PATH",
        help="Mount this GitHub key directly, bypassing agent detection",
    )
    parser.add_argument(
        "--docker",
        metavar="EXECUTABLE",
        help="Docker executable (default: docker from PATH)",
    )
    args = parser.parse_args(argv)
    if not args.install_toolchain and not args.name:
        parser.error("--name is required with --image or --setup-toolchain")
    return args


def main():
    args = parse_args()
    try:
        if args.install_toolchain:
            install_toolchain(Path("package.json"))
            return 0
        docker, env = docker_connection(args.docker)
        if args.setup_toolchain:
            configure_toolchain([docker], env, args.name)
            return 0
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
    except (RuntimeError, OSError, ValueError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    print(f"Started container: {args.name}", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
