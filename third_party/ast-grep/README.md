# `third_party/ast-grep`

Clones [ast-grep](https://github.com/ast-grep/ast-grep) and builds it with the
Rust toolchain Chromium already ships using Chromium's rust toolchain.

## Run

```sh
brave/third_party/ast-grep/build_ast_grep.py
```

Useful flags: `--clean` (wipe source and build dirs, start fresh), `-j N`,
`--verbose`. `--help` lists everything.

## Prerequisites

The Chromium Rust toolchain must be available at
`src/third_party/rust-toolchain/`, so only vanilla Chromium is required.

No other OS-level dependencies are needed — ast-grep's tree-sitter parser builds
use the C compiler already in the developer environment.

## Outputs

| Path                                           | Contents                 |
| ---------------------------------------------- | ------------------------ |
| `third_party/ast-grep-src/`                    | ast-grep source clone    |
| `third_party/ast-grep-intermediate/`           | `cargo-home/`, `target/` |
| `third_party/ast-grep/ast-grep-<os>/bin/ast-grep` | final ast-grep binary |
