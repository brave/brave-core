# Brave checkout image

`Dockerfile` and `build_brave_image.py` build a reusable Linux x64 image with
Brave and dependencies for macOS x64, Windows x64, Linux x64, and Android arm64.
The image includes ripgrep (`rg`) and ast-grep (`ast-grep`) for code searches.

For prerequisites, credentials, Docker selection, image tags, and recovery
instructions, run from the brave-core checkout:

```sh
python3 tools/docker/build_brave_image.py --help
```

## Create a development container

Use the exact image tag printed by the image builder:
`brave-linux-build:<commit-sha>` for a branch or `brave-linux-build:<tag>` for a
tag. The builder does not create a `latest` tag. The example below assumes
`v1.101.39` was built; replace it with your image's tag.

`create_container.py` prefers forwarding the host's SSH agent. It probes
`SSH_AUTH_SOCK` and the Docker Desktop/OrbStack bridge at
`/run/host-services/ssh-auth.sock` through the selected Docker daemon; on macOS
the bridge is tried first. Probe containers are removed after each check. Load
your GitHub key into the host agent with `ssh-add` before running the command.
Passphrase and approval prompts are handled on the host, and the private key is
not mounted in agent mode.

The configured GitHub identity's public key is preferred when available;
otherwise the agent's first identity is used for authentication and signing.
Load the configured key into the agent so it can be selected. Only the selected
public key is written to the container. If no usable agent is found, the helper
resolves the key from `~/.ssh/config` and mounts it read-only. `--ssh-key PATH`
explicitly selects direct key mounting and bypasses agent detection. Direct key
mounting requires a key usable without an agent or passphrase prompt.

Both modes mount `~/.ssh/known_hosts` and the mTLS certificate and key
read-only, then configure GitHub authentication and SSH commit signing.
`known_hosts` must contain verified GitHub host keys. Credential files must be
readable by the image's `builder` user and accessible on the daemon's host.
Remote-build credentials default to `~/.reclient/engflow.crt` and
`~/.reclient/engflow.key`; `BRAVE_RBE_CERT` and `BRAVE_RBE_KEY` override those
locations.

```bash
brave_image=brave-linux-build:v1.101.39
brave_container=brave-dev
brave_docker=(docker)
python3 tools/docker/create_container.py \
  --image "$brave_image" --name "$brave_container"

# To mount a key directly, add --ssh-key /path/to/github-key to the command.
# See all requirements and options:
python3 tools/docker/create_container.py --help
```

The command honors `DOCKER_HOST` and `DOCKER_CONTEXT`; use the same endpoint for
later Docker commands. It supplies `/dev/fuse`, `SYS_ADMIN`, and the AppArmor
setting required by the Windows SDK hook. It leaves the image's checkout
unchanged. Checkout selection, sync, and builds are separate steps. Stopping and
restarting the container preserves its checkout and build output. The chosen SSH
mounts also persist: reusing a container retains its agent or direct-key mode.
Agent containers require the original socket and selected identity to remain
available; changing SSH mounts requires a new container.

The image entrypoint restores the Windows SDK's case-insensitive FUSE mount on
every start when the cached SDK is present. It skips recovery during the initial
image-build startup, before initialization hooks install the SDK. Startup does
not sync or build sources. The FUSE device and capabilities are required when
starting a container with a cached SDK, even for other targets.

The image already configures remote execution in `/workspace/src/brave/.env`,
with `use_remoteexec=true`, service `rbe.ba.brave.com:443`, and the mTLS paths
under `/run/brave-rbe`. It also sets `SISO_CREDENTIAL_HELPER=mTLS`.
Reclient/Siso use the mounted credentials; no credential contents are copied
into the image.

## Configure Git and build

The helper configures GitHub HTTPS remotes to use SSH. Verify the remote-build
mounts and GitHub repository access:

```bash
"${brave_docker[@]}" exec --workdir /workspace/src/brave \
  "$brave_container" bash -c '
    set -euo pipefail
    test -r "$RBE_tls_client_auth_cert"
    test -r "$RBE_tls_client_auth_key"
    git ls-remote origin HEAD
  '
```

For commits, supply your Git identity from the host checkout. SSH signing is
already configured by the helper; register the selected public key with GitHub
as a signing key for verified signatures:

```bash
brave_git_name=$(git config user.name)
brave_git_email=$(git config user.email)
"${brave_docker[@]}" exec --workdir /workspace/src/brave \
  "$brave_container" git config --global user.name "$brave_git_name"
"${brave_docker[@]}" exec --workdir /workspace/src/brave \
  "$brave_container" git config --global user.email "$brave_git_email"

```

The checkout initially contains the image's ref. If you select a different ref,
synchronize all the platforms baked into the image so sync keeps their cached
packages. Then build the desired platform and target:

```bash
# Run after selecting a different ref inside the container.
"${brave_docker[@]}" exec --workdir /workspace/src/brave --env CI=true \
  "$brave_container" pnpm run sync \
  --target_os linux,ios,mac,win,android --target_arch x64,arm64

"${brave_docker[@]}" exec --workdir /workspace/src/brave \
  "$brave_container" brave-build Component \
  --target_os linux --target_arch x64 --target brave

# Open a shell, or stop and later restart the persistent container.
"${brave_docker[@]}" exec --workdir /workspace/src/brave -it \
  "$brave_container" bash
"${brave_docker[@]}" stop "$brave_container"
"${brave_docker[@]}" start "$brave_container"
```

`brave-build` forwards arguments to `pnpm run build`. Android falls back to a
Static build when Component is selected. The image's default command keeps the
container running between `exec` calls.

## Verify the tooling

Run the tooling tests without Docker or remote build credentials:

```sh
python3 -m unittest discover -s tools/docker -p 'test_*.py'
```
