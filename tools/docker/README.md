# Brave checkout image

`Dockerfile` and `build_brave_image.py` build a reusable Linux x64 image with a
Brave checkout and dependencies for macOS x64, Windows x64, Linux x64, and
Android arm64 builds. The script uses only Python's standard library and can be
copied alongside the Dockerfile and `.dockerignore`.

## Build the image

Run on a Linux or macOS host with Python 3.11 or newer, Docker with BuildKit,
and access to Brave's remote build service. The Docker daemon needs an x64 Linux
environment with `/dev/fuse`; the Windows SDK hook requires FUSE and `SYS_ADMIN`
with an unconfined AppArmor profile. Cross-build dependencies include platform
SDKs fetched by the checkout hooks.

From the brave-core checkout:

```sh
python3 tools/docker/build_brave_image.py
python3 tools/docker/build_brave_image.py v1.101.39
```

The optional positional argument selects an exact branch or tag (default:
`master`). Branch images are tagged `brave-linux-build:<commit-sha>`; tag images
are tagged `brave-linux-build:<tag>`. The script initializes all target
operating systems, installs Chromium build dependencies, and requests four
`Component` builds before committing the image. Android falls back to `Static`
because component builds are unsupported there. A failed build retains its
container for inspection and resumption at the original commit. Successful
builds remove only that image-build container.

`--docker EXECUTABLE` selects a Docker executable. `DOCKER_HOST` and
`DOCKER_CONTEXT` select the daemon using Docker's normal precedence.
`--git-cache-path PATH` seeds a new image from an existing depot_tools Git cache
on the Docker host; retained containers keep their current cache.

## Credentials and paths

Provide readable mTLS credentials at `~/.reclient/engflow.crt` and
`~/.reclient/engflow.key`, or override their locations with `BRAVE_RBE_CERT` and
`BRAVE_RBE_KEY`. Credentials are mounted read-only at runtime under
`/run/brave-rbe` and are excluded from the committed image. The build context
allows only the Dockerfile; no credential contents belong in this directory or
the optional Git cache.

The `builder` user is an image-local account with UID/GID selected from the
host. Fixed container paths are `/workspace/src/brave` for the checkout,
`/home/builder/.cache/brave` for caches, and `/usr/local/bin/brave-build` and
`/usr/local/bin/brave-linux-build` for entry points. Host credential paths are
resolved from the current user's home directory or environment overrides.

Run further builds inside a container based on the resulting image, from
`/workspace/src/brave`, with the same credential mounts. `brave-build` forwards
its arguments to `pnpm run build`; `brave-linux-build --sync` synchronizes the
checkout. The image's default command keeps the container running.

When an agent environment is detected, build output is saved to a temporary
`brave-image-*.log` file in the current working directory. Interactive runs
stream output directly.

## Verify the tooling

```sh
python3 -m unittest discover -s tools/docker -p 'test_*.py'
```

The tests exercise Docker selection, argument forwarding, credential mounts,
image publication and failure handling, portable execution, and log capture
without requiring Docker or remote build credentials.
