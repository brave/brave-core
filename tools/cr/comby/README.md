# `tools/cr/comby`

Builds a project-local [comby](https://comby.dev) without touching the
system. Downloads opam from a pinned GitHub release tarball, cold-bootstraps
it (vendoring its own OCaml compiler), then uses that opam to clone and
build comby. Nothing is installed system-wide.

## Run

```sh
brave/tools/cr/comby/build_comby.py
```

Useful flags: `--clean` (start fresh), `--skip-opam` / `--skip-comby` (rebuild
one half in isolation), `-j N`, `--verbose`. Pass `--help` for the full list.

## System dependencies

The opam cold-bootstrap and comby's build both need OS-level libraries.
Install these before running the script (the script does not invoke `sudo`
on your behalf).

**Linux (Debian/Ubuntu):**

```sh
sudo apt-get install autoconf m4 pkg-config zlib1g-dev \
    libpcre3-dev libgmp-dev libev4 libsqlite3-dev libev-dev
```

**macOS:**

```sh
brew install pkg-config gmp pcre libev
```

**Windows:** not supported — build under WSL using the Linux instructions.

## Outputs

| Path | Contents |
| --- | --- |
| `third_party/comby-src/` | comby source clone |
| `third_party/comby-toolchain-intermediate/` | opam source, opam install, `OPAMROOT` |
| `third_party/comby-toolchain/bin/comby` | final comby binary |
