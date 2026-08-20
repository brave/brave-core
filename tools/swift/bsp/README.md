# Swift editor support for GN-built Swift code

Gives editors that use
[sourcekit-lsp](https://github.com/swiftlang/sourcekit-lsp) (VS Code's
[Swift extension](https://github.com/swiftlang/vscode-swift), Neovim, Emacs,
Zed, ...) working autocomplete, jump-to-definition and diagnostics for Swift
files compiled by GN/ninja — including symbols coming from imported modules such
as `BraveCore` and C++ modules exposed through `cxx_import_deps`.

This does not cover `ios/brave-ios`, which is built by Xcode; use a tool like
[xcode-build-server](https://github.com/SolaWing/xcode-build-server) for that.

## Setup

1.  Build the Swift targets you want editor support for at least once. The
    compiler arguments come from the generated ninja files, and imported modules
    have to exist on disk before they can be resolved.

2.  Create `buildServer.json` in `src/brave` (the directory the editor is opened
    at). It is ignored by git, so it can hold absolute local paths:

    ```json
    {
      "name": "brave-gn-swift-build-server",
      "version": "0.1.0",
      "bspVersion": "2.2.0",
      "languages": ["swift"],
      "argv": [
        "/abs/path/to/src/brave/tools/swift/bsp/gn_bsp_server.py",
        "--source-root",
        "/abs/path/to/src",
        "--out-dir",
        "out/ios_current_link"
      ]
    }
    ```

    `argv` is executed without a shell, so every path must be absolute.

3.  Reopen the editor. It launches `gn_bsp_server.py` on its own and asks it for
    compiler arguments whenever a `.swift` file is opened.

## Which targets are exposed

Only targets sources under the workspace root are advertised to sourcekit-lsp,
because it background indexes everything it is told about, which would mean
typechecking upstream code nobody opened and filling its module cache.

Upstream files are **not** cut off by this. When one is opened, sourcekit-lsp
asks for its settings anyway, and the server resolves it against all targets in
the build, so it still gets the real build arguments. The filter only controls
what gets indexed up front.

To index everything, point `--workspace-root` at the source root in
`buildServer.json`:

```json
"--workspace-root", "/abs/path/to/src"
```

## How it works

```
editor -> sourcekit-lsp -> gn_bsp_server.py -> generated ninja files
             LSP              BSP
```

`sourcekit-lsp` supports the [Build Server Protocol][bsp] to obtain build
settings from an external build system. `gn_bsp_server.py` implements the subset
of it that sourcekit-lsp needs, backed by `gn_swift_args.py`.

Recovering the swiftc arguments is not completely obvious, because GN does not
run swiftc directly: the `swift` ninja rule runs
`//build/toolchain/apple/swiftc.py`, which assembles the real command line. So
`gn_swift_args.py`:

1. Scans `out/<dir>/obj/**/*.ninja` for `swift` build edges to discover Swift
   targets, their sources and their ninja variables. (`gn desc` would be the
   obvious source of this, but it needs a full graph load per invocation, which
   is far too slow at Chromium scale.)
2. Expands the `rule swift` command template from `toolchain.ninja` with those
   variables to reconstruct the `swiftc.py` invocation.
3. Runs `swiftc.py` as a subprocess with `--swift-toolchain-path` pointed at a
   fake toolchain whose `swiftc` records the arguments it was given and exits
   non-zero. `swiftc.py` builds the compiler path as
   `<toolchain>/usr/bin/swiftc`, so this intercepts the invocation through its
   documented command line interface only, and stops it before the post-compile
   steps that read real build outputs. `--target-out-dir` and `--header-path`
   are also overridden to point inside `swift_lsp_cache/`, so the bookkeeping
   files swiftc.py writes before that point (its `OutputFileMap.json` and
   `SwiftFileList`) land in the cache instead of the real build directory.
4. Strips code generation flags (`-c`, `-emit-*`, `-output-file-map`, ...) and
   expands the `@Module.SwiftFileList` response file, leaving arguments that
   only describe how to parse and typecheck the module.

Results are cached in memory and invalidated when `build.ninja`'s mtime changes,
so re-running `gn gen` is picked up automatically.

Intermediate state the compiler needs (module cache, precompiled headers, index
store) is written to `out/<dir>/swift_lsp_cache/`, keeping it out of the way of
the real build.

The clang module cache is shared across targets, in
`swift_lsp_cache/SharedModuleCache.noindex/`. swiftc.py gives each target its
own (correct for a build, where rebuilding one target should not disturb
another) but entries are keyed by a hash of the compilation settings, so targets
built with the same flags produce byte-identical modules and can reuse each
other's work, while targets with differing flags coexist under different hashes.
Measured over 23 targets in an iOS build, typechecking all of them from cold
went from 153s to 77s, and the cache from 1376 MB to 563 MB.

Two things are deliberately **not** shared:

- `-pch-output-dir`. Bridging header PCHs are named
  `<header basename>-<hash>.pch`, and that hash does not distinguish two targets
  with different `bridge_header.h` files, so sharing makes them collide and
  emitting the PCH fails outright.
- `-index-store-path`. Kept per-target so index units cannot be attributed to
  the wrong target.

swiftc creates neither of these directories itself and fails if they are
missing, so they are recreated on every request rather than only at extraction
time — otherwise deleting `swift_lsp_cache` to reclaim disk would break every
open file until the editor was restarted.

## Coupling to Chromium, and how it is contained

Nothing here is a supported extension point, so the parts that depend on code we
do not control are deliberately narrow and fail loudly:

- **swiftc.py's interface.** Five things are assumed: the file exists, it
  accepts `--swift-toolchain-path`, it runs `<toolchain>/usr/bin/swiftc`, and it
  accepts `--target-out-dir`/`--header-path` (all documented flags). The first
  three are checked before use, with an error naming this directory. No internal
  function or variable is touched, so upstream refactors of how the command line
  is assembled do not matter — that logic is executed rather than reimplemented,
  so the arguments follow it automatically.
- **Build isolation.** swiftc.py writes its own bookkeeping files under
  `--target-out-dir`/`--header-path` before the fake `swiftc` stops it, so both
  are redirected into `swift_lsp_cache/` (see step 3). Nothing swiftc.py writes
  can reach the real build directory regardless of which flags it assembles, so
  this holds even if upstream adds new output flags. `_DROPPED_FLAGS` and
  friends still strip codegen flags, but only to keep the argument list to what
  describes parsing and typechecking; they are no longer load-bearing for
  isolation, so a missed one is harmless rather than a write into the build.
- **The ninja file format.** The `swift` build edge shape and the `rule swift`
  command template are parsed. `gn_swift_args_test.py` covers this.

What none of the above can catch is arguments that are still well-formed but no
longer describe how the module is built. `gn_swift_args_test.py` catches that by
typechecking real targets with the extracted arguments:

```sh
python3 tools/swift/bsp/gn_swift_args_test.py --out-dir out/ios_current_link
```

Worth running after a Chromium roll that touches `build/toolchain/apple/` or
`build/config/apple/swift_source_set.gni`.

## Debugging

`gn_swift_args.py` runs standalone, which is the fastest way to tell whether a
problem is in the argument extraction or in the editor integration:

```sh
# List all Swift targets in an output directory.
python3 tools/swift/bsp/gn_swift_args.py --out-dir out/ios_current_link

# Print the arguments for one file.
python3 tools/swift/bsp/gn_swift_args.py --out-dir out/ios_current_link \
    ios/swift/foo.swift

# Verify the arguments actually typecheck the file.
python3 tools/swift/bsp/gn_swift_args.py --out-dir out/ios_current_link \
    ios/swift/foo.swift --json > /tmp/args.json
(cd ../out/ios_current_link && xcrun swiftc -typecheck $(python3 -c \
    'import json, shlex; print(shlex.join(json.load(open("/tmp/args.json"))))'))
```

The server logs to stderr, which the VS Code Swift extension shows in its
"SourceKit Language Server" output channel. Set `BRAVE_BSP_DEBUG=1` in the
environment for per-request logging.

Common failures:

- **No completions at all, no server output.** The editor did not find
  `buildServer.json`, or a path in `argv` is wrong. `argv` is not run through a
  shell and `~` is not expanded.
- **"no Swift target owns <file>".** The file is not in any target's `sources`
  in the current output directory. Add it to `BUILD.gn` and re-run `gn gen`.
- **Imports of other modules do not resolve.** That module has not been built in
  this output directory yet.

## Compatibility

The sourceKit BSP extensions are experimental and change between toolchain
versions. This was developed against the Swift 6.3 toolchain shipped with
Xcode 26. If a toolchain upgrade breaks things, check the [BSP extensions
documentation][bsp-extensions] for the corresponding sourcekit-lsp release.

[bsp]: https://build-server-protocol.github.io/docs/specification
[bsp-extensions]:
  https://github.com/swiftlang/sourcekit-lsp/blob/main/Contributor%20Documentation/BSP%20Extensions.md
