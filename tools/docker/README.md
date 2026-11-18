# Brave checkout image

`Dockerfile` and `build_brave_image.py` build a reusable Linux x64 image with
Brave and dependencies for macOS x64, Windows x64, Linux x64, and Android arm64.

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

Run these commands in Bash on the host. Adjust the SSH key path to the identity
used for GitHub in your host SSH configuration. The example selects that key
directly and does not use the host SSH configuration or an SSH agent. The key
must work in batch mode without a passphrase prompt. `known_hosts` must already
contain verified GitHub host keys. Use absolute source paths. All four
credential files must be readable by the image's `builder` user, whose UID/GID
were selected when the image was built, and accessible at these source paths on
the Docker daemon's host.

```bash
set -euo pipefail
brave_image=brave-linux-build:v1.101.39
brave_container=brave-dev
brave_rbe_cert="${BRAVE_RBE_CERT:-$HOME/.reclient/engflow.crt}"
brave_rbe_key="${BRAVE_RBE_KEY:-$HOME/.reclient/engflow.key}"
brave_ssh_key="$HOME/.ssh/id_ed25519"
brave_known_hosts="$HOME/.ssh/known_hosts"
brave_docker=(docker)
brave_ssh_command='ssh -F /dev/null -i /home/builder/.ssh/github_key'
brave_ssh_command+=' -o IdentitiesOnly=yes -o IdentityAgent=none'
brave_ssh_command+=' -o AddKeysToAgent=no -o BatchMode=yes'
brave_ssh_command+=' -o StrictHostKeyChecking=yes'
brave_ssh_command+=' -o UserKnownHostsFile=/home/builder/.ssh/known_hosts'

for brave_file in "$brave_rbe_cert" "$brave_rbe_key" \
  "$brave_ssh_key" "$brave_known_hosts"; do
  test -f "$brave_file"
  test -r "$brave_file"
done

"${brave_docker[@]}" create \
  --name "$brave_container" \
  --init --platform=linux/amd64 --shm-size=4g \
  --device /dev/fuse --cap-add SYS_ADMIN \
  --security-opt apparmor=unconfined \
  --mount "type=bind,source=$brave_rbe_cert,target=/run/brave-rbe/engflow.crt,readonly" \
  --mount "type=bind,source=$brave_rbe_key,target=/run/brave-rbe/engflow.key,readonly" \
  --mount "type=bind,source=$brave_ssh_key,target=/home/builder/.ssh/github_key,readonly" \
  --mount "type=bind,source=$brave_known_hosts,target=/home/builder/.ssh/known_hosts,readonly" \
  --env "GIT_SSH_COMMAND=$brave_ssh_command" \
  "$brave_image"
"${brave_docker[@]}" start "$brave_container"
```

`brave_docker` honors `DOCKER_HOST` and `DOCKER_CONTEXT`. To select an endpoint
explicitly, add `--context CONTEXT` or `--host ENDPOINT` to that array before
creating the container, and use the same array for subsequent commands.
`/dev/fuse`, `SYS_ADMIN`, and the AppArmor setting support the Windows SDK's
case-insensitive filesystem hook. The container keeps its checkout and build
outputs in its writable layer; stopping and restarting it preserves them.

The image already configures remote execution in `/workspace/src/brave/.env`,
with `use_remoteexec=true`, service `rbe.ba.brave.com:443`, and the certificate
and key paths used above. It also sets `SISO_CREDENTIAL_HELPER=mTLS`.
Reclient/Siso use the mounted mTLS files; no credential contents are copied into
the image. The SSH mounts supply Git authentication separately.

## Configure Git and build

Configure Git to use the mounted SSH identity for GitHub HTTPS remotes, then
verify both the remote-build mounts and GitHub repository access:

```bash
"${brave_docker[@]}" exec --workdir /workspace/src/brave \
  "$brave_container" bash -c '
    set -euo pipefail
    test -r "$RBE_tls_client_auth_cert"
    test -r "$RBE_tls_client_auth_key"
    git config --global core.sshCommand "$GIT_SSH_COMMAND"
    git config --global url."ssh://git@github.com/".insteadOf https://github.com/
    git ls-remote origin HEAD
  '
```

For commits, supply your Git identity from the host checkout. To sign commits
with the mounted key, register its public key with GitHub as a signing key and
configure SSH signing inside the container:

```bash
brave_git_name=$(git config user.name)
brave_git_email=$(git config user.email)
"${brave_docker[@]}" exec --workdir /workspace/src/brave \
  "$brave_container" git config --global user.name "$brave_git_name"
"${brave_docker[@]}" exec --workdir /workspace/src/brave \
  "$brave_container" git config --global user.email "$brave_git_email"

# Optional SSH commit signing.
"${brave_docker[@]}" exec --workdir /workspace/src/brave \
  "$brave_container" bash -c '
    set -euo pipefail
    git config --global gpg.format ssh
    git config --global user.signingkey /home/builder/.ssh/github_key
    git config --global commit.gpgsign true
  '
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
