# `tools/cr/ast_grep`

Clones [ast-grep](https://github.com/ast-grep/ast-grep) and builds it with the
Rust toolchain Chromium already ships under `src/third_party/rust-toolchain/`.
Nothing is fetched from rustup and nothing escapes the brave-core checkout —
`CARGO_HOME` and `CARGO_TARGET_DIR` are both pinned under
`third_party/ast-grep-toolchain-intermediate/`.

## Run

```sh
brave/tools/cr/ast_grep/build_ast_grep.py
```

Useful flags: `--clean` (wipe source and build dirs, start fresh), `-j N`,
`--verbose`. `--help` lists everything.

## Prerequisites

The Chromium Rust toolchain must be available at
`src/third_party/rust-toolchain/`. A normal `gclient sync` brings it down
automatically; if you've never synced or want a custom toolchain, run
`tools/rust/build_rust.py` from the Chromium tree. The script aborts with a
clear message if `cargo` / `rustc` aren't where it expects them.

No other OS-level dependencies are needed — ast-grep's tree-sitter parser builds
use the C compiler already in the developer environment.

## Outputs

| Path                                           | Contents                 |
| ---------------------------------------------- | ------------------------ |
| `third_party/ast-grep-src/`                    | ast-grep source clone    |
| `third_party/ast-grep-toolchain-intermediate/` | `cargo-home/`, `target/` |
| `third_party/ast-grep-toolchain/bin/ast-grep`  | final ast-grep binary    |
