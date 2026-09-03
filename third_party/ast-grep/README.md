# `third_party/ast-grep`

Clones [ast-grep](https://github.com/ast-grep/ast-grep) and builds it with the
Rust toolchain Chromium already ships using Chromium's rust toolchain. Also
compiles [tree-sitter-gn](https://github.com/tree-sitter-grammars/tree-sitter-gn)
into a shared library ast-grep loads at runtime, via the `tree_sitter_gn` target
in `BUILD.gn`. Nothing in the browser build depends on that target, so the script
generates a dedicated out dir for it with `root_extra_deps`.

## Run

```sh
brave/third_party/ast-grep/build_ast_grep.py
```

Useful flags: `--clean` (wipe source and build dirs, start fresh), `-j N`,
`--verbose`. `--help` lists everything.

## Prerequisites

The Chromium Rust toolchain must be available at
`src/third_party/rust-toolchain/`, and `gn` on `PATH` (depot_tools) for the
grammar, so only a synced checkout is required.

## Outputs

| Path                                                 | Contents                     |
| ----------------------------------------------------- | ---------------------------- |
| `third_party/ast-grep-src/`                          | ast-grep source clone         |
| `third_party/tree-sitter-gn-src/`                    | tree-sitter-gn source clone   |
| `third_party/ast-grep-intermediate/`                 | `cargo-home/`, `target/`      |
| `src/out/ast-grep-tree-sitter-gn/`                   | grammar-only GN build dir     |
| `third_party/ast-grep/ast-grep-<os>/bin/ast-grep`    | final ast-grep binary         |
| `third_party/ast-grep/ast-grep-<os>/lib/gn.<ext>`    | `gn` custom-language library  |
| `third_party/ast-grep/ast-grep-<os>/sgconfig.yml`    | registers `gn` for ast-grep   |
