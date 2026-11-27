#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

"""Build a reusable Linux x64 Brave checkout image from a branch or tag.

Requirements:
  Run on a Linux or macOS host with Python 3.11 or newer, Docker with BuildKit,
  and access to Brave's remote build service. The Docker daemon needs an x64
  Linux environment with /dev/fuse. The Windows SDK hook requires SYS_ADMIN
  and an unconfined AppArmor profile. Platform SDKs are fetched by the hooks.
  Keep this script, create_container.py, Dockerfile, and .dockerignore together.

Builds and image tags:
  REF selects an exact brave-core branch or tag (default: master).
  Branch images are tagged brave-linux-build:<commit-sha>; tag images are
  tagged brave-linux-build:<tag>. Initialization covers all target operating
  systems, installs Chromium build dependencies, and requests Component builds
  for macOS x64, Windows x64, Linux x64, and Android arm64. Android falls back
  to Static because component builds are unsupported there. The image is
  committed only after all four builds succeed.

Credentials and Docker selection:
  Provide readable mTLS credentials at ~/.reclient/engflow.crt and
  ~/.reclient/engflow.key, or override them with BRAVE_RBE_CERT and
  BRAVE_RBE_KEY. Credentials are mounted read-only under /run/brave-rbe at
  runtime and excluded from the committed image. Host paths are resolved from
  the current user's home directory or environment overrides. The build
  context allows only the Dockerfile and its tooling; keep credentials out of
  this directory and the optional Git cache.
  Docker defaults to the executable on PATH; --docker selects another one.
  DOCKER_HOST and DOCKER_CONTEXT select the daemon using Docker's precedence.
  Credential files and any Git cache must be accessible to that daemon.
  --git-cache-path seeds a new image from an existing depot_tools Git cache;
  retained containers keep their current cache.

Failure recovery and output:
  Concurrent invocations for the same daemon/container under the same host
  user are rejected by a standard-library flock. It is held from inspection
  through publication and cleanup, and released on exit or process death.
  Lock files remain in ~/.cache/brave/image-locks; their presence does not
  indicate ownership. Retained containers with active Docker exec sessions
  cannot be resumed until that work finishes, even if its launcher has exited.
  Failed builds retain their container for inspection. Rerunning the same REF
  resumes at its original commit, even if the branch has advanced. Successful
  builds remove only that image-build container.
  Agent runs save output to brave-image-*.log in the current working directory
  and print the log path and exit status. Interactive runs stream output.

Container layout and further builds:
  builder is an image-local user with the host's UID/GID. The checkout is at
  /workspace/src/brave and caches are under /home/builder/.cache/brave.
  Create a container from the resulting image with the same credential mounts,
  then run /usr/local/bin/brave-build from /workspace/src/brave; arguments are
  forwarded to pnpm run build. /usr/local/bin/brave-linux-build --sync
  synchronizes the checkout. The default command keeps the container running.
  Node and pnpm follow the checked-out package.json, selecting the minimum
  supported stable versions before initialization rather than fixed image tags.

Examples:
  python3 build_brave_image.py
  python3 build_brave_image.py v1.101.39
  python3 build_brave_image.py master --git-cache-path ~/.cache/brave/git
"""

import argparse
from contextlib import contextmanager, redirect_stderr, redirect_stdout
import errno
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import traceback

SUPPORT = Path(__file__).resolve().parent

REPOSITORY = "https://github.com/brave/brave-core.git"
PLATFORMS = (
    ("mac", "x64"),
    ("win", "x64"),
    ("linux", "x64"),
    ("android", "arm64"),
)
IMAGE_FORMAT = "6"


def selected_platforms(requested=None, previous=None):
    """Use requested or saved development platforms, defaulting to Linux x64."""
    previous = previous or {}
    selections = requested or previous.get("last_platforms")
    if not selections and previous.get("last_build"):
        last = previous["last_build"]
        selections = [(last.get("target_os"), last.get("target_arch"))]
    if not selections:
        selections = [("linux", "x64")]
    if any(len(pair) != 2 or not all(pair) for pair in selections):
        raise RuntimeError(
            "Invalid platform selection; supply --target_platform OS-ARCH"
        )
    return list(dict.fromkeys(tuple(pair) for pair in selections))


@contextmanager
def image_lock(command, env, name):
    """Hold local ownership without relying on a removable sentinel file."""
    daemon = subprocess.check_output(
        [*command, "info", "--format", "{{.ID}}"], text=True, env=env
    ).strip()
    if not daemon:
        raise RuntimeError("Cannot determine the Docker daemon's lock identity")
    identity = hashlib.sha256((daemon + "\0" + name).encode()).hexdigest()
    directory = Path.home() / ".cache/brave/image-locks"
    directory.mkdir(mode=0o700, parents=True, exist_ok=True)
    # Keep the inode: unlinking lets another invocation lock a different file.
    with (directory / (identity + ".lock")).open("a", encoding="utf-8") as lock:
        try:
            fcntl.flock(lock.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError as error:
            if error.errno not in (errno.EACCES, errno.EAGAIN):
                raise
            raise RuntimeError(
                f"Image build already running for {name} on this Docker daemon"
            ) from None
        yield


def docker_binary(executable=None):
    """Resolve the selected Docker executable, defaulting to docker on PATH."""
    command = (
        os.path.expanduser(executable) if executable is not None else "docker"
    )
    binary = shutil.which(command)
    if binary:
        return str(Path(binary).absolute())
    raise RuntimeError(
        f"Docker executable {command!r} was not found or is not executable"
    )


def docker_connection(executable=None, *, env=None):
    """Check Docker availability and return its executable and environment."""
    docker = docker_binary(executable)
    env = dict(os.environ if env is None else env)
    env["PATH"] = str(Path(docker).parent) + os.pathsep + env.get("PATH", "")
    subprocess.run(
        [docker, "info", "--format", "{{.OSType}}"], check=True, env=env
    )
    return docker, env


def inspect(docker, env, kind, name):
    """Return a Docker object's metadata, or None if inspection fails."""
    result = subprocess.run(
        [docker, kind, "inspect", name],
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if result.returncode:
        return None
    return json.loads(result.stdout)[0]


def credentials():
    """Return readable remote-build credentials, honoring host overrides."""
    certificate = (
        Path(os.environ.get("BRAVE_RBE_CERT", "~/.reclient/engflow.crt"))
        .expanduser()
        .resolve()
    )
    key = (
        Path(os.environ.get("BRAVE_RBE_KEY", "~/.reclient/engflow.key"))
        .expanduser()
        .resolve()
    )
    for path in (certificate, key):
        if not path.is_file() or not os.access(path, os.R_OK):
            raise RuntimeError(
                f"Remote Siso credential is missing or unreadable: {path}"
            )
    return certificate, key


def credential_mounts(image, certificate, key):
    """Mount credentials at the destinations defined by the Dockerfile."""
    environment = dict(
        item.split("=", 1) for item in image["Config"].get("Env", [])
    )
    arguments = []
    for source, setting in (
        (certificate, "RBE_tls_client_auth_cert"),
        (key, "RBE_tls_client_auth_key"),
    ):
        target = environment.get(setting)
        if not target or not target.startswith("/"):
            raise RuntimeError(
                f"Image has no absolute {setting} credential path; rebuild it"
            )
        arguments.extend(
            ["--mount", f"type=bind,source={source},target={target},readonly"]
        )
    return arguments


def verify_checkout(execute, ref, ref_type="branch", commit=None, env=None):
    """Verify a named branch or a detached tag before reuse or compilation."""
    symbolic = subprocess.run(
        [*execute, "git", "symbolic-ref", "--quiet", "HEAD"],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
    )
    if ref_type == "branch":
        if (
            symbolic.returncode
            or symbolic.stdout.strip() != f"refs/heads/{ref}"
        ):
            raise RuntimeError(f"Container is not on branch {ref!r}")
        return None
    if ref_type != "tag":
        raise RuntimeError(f"Unsupported checkout reference type: {ref_type!r}")
    if symbolic.returncode != 1:
        raise RuntimeError(f"Tag checkout {ref!r} must have detached HEAD")
    active = subprocess.check_output(
        [*execute, "git", "rev-parse", "HEAD"], text=True, env=env
    ).strip()
    tagged = subprocess.check_output(
        [*execute, "git", "rev-parse", f"refs/tags/{ref}^{{commit}}"],
        text=True,
        env=env,
    ).strip()
    if active != tagged or (commit is not None and active != commit):
        raise RuntimeError(
            f"Container is not at the saved commit for tag {ref!r}"
        )
    return active


def build_container(docker, identifier, path, build_args, *, env=None):
    """Invoke the image's build command in an already prepared container."""
    subprocess.run(
        [
            *docker,
            "exec",
            "--workdir",
            path,
            identifier,
            "/usr/local/bin/brave-build",
            *build_args,
        ],
        check=True,
        env=env,
    )


def resolve_reference(ref):
    """Resolve an exact branch or tag, without falling back to master."""
    if ref.startswith("-"):
        raise RuntimeError("Reference names cannot start with '-'")
    subprocess.run(["git", "check-ref-format", f"refs/heads/{ref}"], check=True)
    refs = dict(
        line.split()[::-1]
        for line in subprocess.check_output(
            [
                "git",
                "ls-remote",
                REPOSITORY,
                f"refs/heads/{ref}",
                f"refs/tags/{ref}",
                f"refs/tags/{ref}^{{}}",
            ],
            text=True,
        ).splitlines()
    )
    branch, tag = refs.get(f"refs/heads/{ref}"), refs.get(f"refs/tags/{ref}")
    if branch and tag:
        raise RuntimeError(
            f"Reference {ref!r} identifies both a branch and a tag"
        )
    if branch:
        return "branch", branch
    if tag:
        return "tag", refs.get(f"refs/tags/{ref}^{{}}", tag)
    raise RuntimeError(f"Cannot resolve brave-core branch or tag {ref!r}")


def build_checkout(command, env, seed, head, git_cache_path=None):
    """Build the bootstrap checkout, optionally copying a prepared Git cache."""
    arguments = [
        *command,
        "build",
        "--platform=linux/amd64",
        "--target",
        "checkout-cached" if git_cache_path is not None else "checkout",
        "--tag",
        seed,
        "--build-arg",
        f"BUILD_UID={os.getuid()}",
        "--build-arg",
        f"BUILD_GID={os.getgid()}",
        "--build-arg",
        f"BRAVE_HEAD={head}",
    ]
    if git_cache_path is not None:
        arguments.extend(["--build-context", f"git_cache={git_cache_path}"])
    subprocess.run([*arguments, str(SUPPORT)], check=True, env=env)


def configure_toolchain(docker, env, identifier):
    """Install from the current container checkout, including older images."""
    directory = "/usr/local/lib/brave-docker"
    subprocess.run(
        [
            *docker,
            "exec",
            "--user",
            "root",
            identifier,
            "mkdir",
            "-p",
            directory,
        ],
        check=True,
        env=env,
    )
    for name in ("create_container.py", "build_brave_image.py"):
        subprocess.run(
            [
                *docker,
                "cp",
                str(Path(__file__).with_name(name)),
                f"{identifier}:{directory}/{name}",
            ],
            check=True,
            env=env,
        )
    subprocess.run(
        [
            *docker,
            "exec",
            "--user",
            "root",
            "--workdir",
            "/workspace/src/brave",
            identifier,
            "python3",
            directory + "/create_container.py",
            "--install-toolchain",
        ],
        check=True,
        env=env,
    )


def build_image(ref="master", git_cache_path=None, *, docker_executable=None):
    """Sync and build a dedicated container, then publish and remove it."""
    docker, env = docker_connection(docker_executable)
    context = subprocess.check_output(
        [docker, "context", "show"], text=True, env=env
    ).strip()
    endpoint = (
        ["--host", env["DOCKER_HOST"]]
        if env.get("DOCKER_HOST") and not env.get("DOCKER_CONTEXT")
        else ["--context", context]
    )
    command = [docker, *endpoint]
    # Pin inspection to the same daemon even if the default context changes.
    if endpoint[0] == "--host":
        env.pop("DOCKER_CONTEXT", None)
        env["DOCKER_HOST"] = endpoint[1]
    else:
        env.pop("DOCKER_HOST", None)
        env["DOCKER_CONTEXT"] = endpoint[1]
    name = "brave-image-" + hashlib.sha256(ref.encode()).hexdigest()[:16]
    with image_lock(command, env, name):
        _build_image(command, env, ref, git_cache_path)


def _build_image(command, env, ref, git_cache_path):
    """Run the image lifecycle while the caller holds exclusive ownership."""
    if git_cache_path is not None:
        git_cache_path = Path(git_cache_path).expanduser().resolve(strict=True)
        if not git_cache_path.is_dir():
            raise RuntimeError(
                f"Git cache path is not a directory: {git_cache_path}"
            )
    ref_type, head = resolve_reference(ref)
    certificate, key = credentials()
    docker = command[0]
    identity = hashlib.sha256(ref.encode()).hexdigest()[:16]
    name = f"brave-image-{identity}"
    seed = f"brave-linux-build:checkout-{identity}"
    container = inspect(docker, env, "container", name)
    if container is not None:
        if container.get("ExecIDs"):
            raise RuntimeError(
                f"Image container {name} has active Docker exec sessions; "
                "wait for that work to finish before retrying"
            )
        labels = container["Config"].get("Labels") or {}
        if labels.get("io.brave.checkout.format") != IMAGE_FORMAT:
            raise RuntimeError(
                f"Retained image container {name} uses an older build setup; "
                "it was retained for migration"
            )
        retained_head = labels.get("io.brave.image.head")
        retained_ref = ref if ref_type == "tag" else retained_head
        retained_type = "tag" if ref_type == "tag" else "commit"
        if (
            labels.get("io.brave.image.ref"),
            labels.get("io.brave.image.ref_type"),
        ) not in ((ref, ref_type), (retained_ref, retained_type)):
            raise RuntimeError(
                f"Retained image container {name} has a different reference or commit"
            )
        if not retained_head or (ref_type == "tag" and retained_head != head):
            raise RuntimeError(
                f"Retained image container {name} has a different reference or commit"
            )
        # Resume the selected commit even if the branch advanced during the build.
        head = retained_head
        if container["State"].get("Paused"):
            raise RuntimeError(f"Image container {name} is paused")
    image_ref = ref if ref_type == "tag" else head
    image_ref_type = "tag" if ref_type == "tag" else "commit"
    image = f"brave-linux-build:{image_ref}"
    if container is None:
        build_checkout(command, env, seed, head, git_cache_path)
        seed_info = inspect(docker, env, "image", seed)
        if seed_info is None:
            raise RuntimeError(f"Bootstrap image disappeared: {seed}")
        mounts = credential_mounts(seed_info, certificate, key)
        subprocess.run(
            [
                *command,
                "create",
                "--name",
                name,
                "--init",
                "--platform=linux/amd64",
                "--shm-size=4g",
                # Chromium's Windows SDK hook mounts a case-insensitive FUSE filesystem.
                "--device",
                "/dev/fuse",
                "--cap-add",
                "SYS_ADMIN",
                "--security-opt",
                "apparmor=unconfined",
                "--label",
                f"io.brave.image.ref={image_ref}",
                "--label",
                f"io.brave.image.head={head}",
                "--label",
                f"io.brave.image.ref_type={image_ref_type}",
                *mounts,
                seed,
            ],
            check=True,
            env=env,
        )
        container = inspect(docker, env, "container", name)
        if container is None:
            raise RuntimeError(f"Image container disappeared: {name}")
    identifier = container["Id"]
    print(
        f"Reference: {ref} ({ref_type})\nCommit: {head}\nContainer: {identifier}",
        flush=True,
    )
    try:
        if not container["State"]["Running"]:
            subprocess.run([*command, "start", identifier], check=True, env=env)
        execute = [
            *command,
            "exec",
            "--workdir",
            "/workspace/src/brave",
            identifier,
        ]
        select = '''set -euo pipefail
ref=$1
kind=$2
commit=$3
test -z "$(git status --porcelain --untracked-files=no)"
if [ "$kind" = tag ]; then
  git switch --detach "$commit"
else
  git switch -C "$ref" "$commit"
fi
test "$(git rev-parse HEAD)" = "$commit"
'''
        subprocess.run(
            [
                *execute,
                "bash",
                "-c",
                select,
                "prepare-image",
                ref,
                ref_type,
                head,
            ],
            check=True,
            env=env,
        )
        configure_toolchain(command, env, identifier)
        subprocess.run(
            [
                *execute,
                "pnpm",
                "run",
                "init",
                "--target_os",
                "linux,ios,mac,win,android",
                "--target_arch",
                "x64,arm64",
            ],
            check=True,
            env=env,
        )
        verify_checkout(execute, ref, ref_type, head, env=env)
        subprocess.run(
            [
                *command,
                "exec",
                "--user",
                "root",
                "--workdir",
                "/workspace/src",
                identifier,
                "bash",
                "build/install-build-deps.sh",
                "--no-prompt",
                "--android",
            ],
            check=True,
            env=env,
        )
        for platform, arch in PLATFORMS:
            print(f"Building {platform}:{arch}", flush=True)
            build_container(
                command,
                identifier,
                "/workspace/src/brave",
                [
                    "Component",
                    "--target_os",
                    platform,
                    "--target_arch",
                    arch,
                    "-C",
                    f"image_{platform}_{arch}",
                ],
                env=env,
            )
        verify_checkout(execute, ref, ref_type, head, env=env)
        subprocess.run([*command, "stop", identifier], check=True, env=env)
        changes = [
            argument
            for label in (
                f"io.brave.checkout.format={IMAGE_FORMAT}",
                f"io.brave.checkout.head={head}",
                f"io.brave.checkout.ref={image_ref}",
                f"io.brave.checkout.ref_type={image_ref_type}",
                f"io.brave.image.ref={image_ref}",
                f"io.brave.image.ref_type={image_ref_type}",
                "io.brave.checkout.platforms=mac,win,linux,android",
            )
            for argument in ("--change", f"LABEL {label}")
        ]
        subprocess.run(
            [*command, "commit", *changes, identifier, image],
            check=True,
            env=env,
        )
        subprocess.run(
            [*command, "image", "inspect", image],
            check=True,
            env=env,
            stdout=subprocess.DEVNULL,
        )
        subprocess.run([*command, "rm", identifier], check=True, env=env)
    except BaseException:
        print(
            f"Image build failed; container retained: {name}",
            file=sys.stderr,
            flush=True,
        )
        raise
    print(
        f"Built shared checkout image: {image} at {ref} ({head}) for mac, win, linux and android.\n"
        f"Removed image container: {name}. Existing branch containers retain their work.",
        flush=True,
    )


def parse_args():
    """Parse image-build options before agent logging is configured."""
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "ref",
        metavar="REF",
        nargs="?",
        default="master",
        help="Brave branch or tag (default: master)",
    )
    parser.add_argument(
        "--docker",
        metavar="EXECUTABLE",
        help="Docker executable (default: docker from PATH)",
    )
    parser.add_argument(
        "--git-cache-path",
        type=Path,
        help="Copy an existing depot_tools Git cache into the new image container",
    )
    return parser.parse_args()


def _is_agent():
    variables = (
        "CODEX_THREAD_ID",
        "CODEX_SESSION_ID",
        "CODEX_CI",
        "CLAUDECODE",
        "CLAUDE_CODE_REMOTE",
        "CURSOR_AGENT",
        "GEMINI_CLI",
    )
    return any(
        os.environ.get(variable, "").lower() not in ("", "0", "false", "no")
        for variable in variables
    )


def _run_build(args):
    """Return a shell exit status and keep failure details with build output."""
    try:
        build_image(
            args.ref, args.git_cache_path, docker_executable=args.docker
        )
        return 0
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
    except KeyboardInterrupt:
        print("Interrupted", file=sys.stderr)
        return 130
    except Exception:
        traceback.print_exc()
        return 1


def main():
    """Run the standalone image builder, logging agent output in the cwd."""
    args = parse_args()
    if not _is_agent():
        return _run_build(args)

    descriptor, path = tempfile.mkstemp(
        prefix="brave-image-", suffix=".log", dir=Path.cwd()
    )
    print(f"Log: {path}", flush=True)
    sys.stderr.flush()
    saved_stdout, saved_stderr = os.dup(1), os.dup(2)
    with os.fdopen(descriptor, "w", buffering=1) as log:
        try:
            # Docker and other child processes inherit the redirected descriptors.
            os.dup2(log.fileno(), 1)
            os.dup2(log.fileno(), 2)
            with redirect_stdout(log), redirect_stderr(log):
                code = _run_build(args)
                print(f"Exit code: {code}", flush=True)
        finally:
            os.dup2(saved_stdout, 1)
            os.dup2(saved_stderr, 2)
            os.close(saved_stdout)
            os.close(saved_stderr)
    print(f"Exit code: {code}", flush=True)
    return code


if __name__ == "__main__":
    sys.exit(main())
