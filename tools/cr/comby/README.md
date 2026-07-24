# `tools/cr/comby`

Builds a project-local [comby](https://comby.dev) without touching the
system. On Linux / macOS opam is cloned at a pinned tag and cold-bootstrapped
(vendoring its own OCaml compiler); on native Windows opam is fetched as a
prebuilt binary from the same pinned release. Either way, the resulting opam
is then used to clone and build comby into a project-local toolchain.
Nothing is installed system-wide.

`build_comby.py` is self-contained: it depends only on the Python
standard library and `git`, with no sibling-module imports, so it can be
piped straight from GitHub in CI just like the scripts under
[`tools/cr/toolchains/`](../toolchain/README.md).

## Run

From a Brave checkout (writes outputs into `brave/third_party/`):

```sh
tools/cr/comby/build_comby.py
```

Standalone (e.g. CI without a Brave checkout):

```sh
curl -sL \
    https://raw.githubusercontent.com/brave/brave-core/refs/heads/master/tools/cr/comby/build_comby.py \
    | python3 - --third-party-dir=/path/to/third_party/
```

Useful flags: `--third-party-dir PATH` (where the `comby-*` directories
are created — defaults to `brave/third_party/` when run from a checkout,
required otherwise), `--clean` (start fresh), `--skip-opam` /
`--skip-comby` (rebuild one half in isolation), `-j N`, `--verbose`.
Pass `--help` for the full list.

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
brew install autoconf gmp libev pcre pkgconf sqlite
```

**Windows (WSL):** identifies as Linux, so use the Linux instructions
above. WSL builds include `hack_parallel` and the parallel pipeline.

**Windows (native):** opam itself is fetched as a prebuilt binary from
opam's GitHub release page, so `make`/`autoconf`/`m4` are not needed to
get opam running. The OS-level libraries comby's C bindings link against
(`pcre`, `gmp`, `libev`, `sqlite`, `zlib`) plus `pkg-config` still need
to be available -- the simplest way is msys2 (`pacman -S pkg-config
gmp-devel pcre-devel libev-devel zlib-devel sqlite-devel`). `hack_parallel`
is filtered out automatically -- its shared-memory primitives are
Unix-only and unbuildable on Windows -- and comby is compiled with the
`parany`-based fallback in `src/dune` / `lib/app/pipeline/dune`. The
resulting binary is functionally complete, just without the parallel
pipeline.

## Outputs

| Path | Contents |
| --- | --- |
| `third_party/comby-src/` | comby source clone |
| `third_party/comby-toolchain-intermediate/` | opam source, opam install, `OPAMROOT` |
| `third_party/comby-toolchain/bin/comby` | final comby binary |
