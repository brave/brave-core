# Contributing to `walrus`

## Build

To build `walrus`, run:

```
cargo build
```

## Test

The tests rely on [WABT][] being installed on your system's `$PATH`, so make
sure you have that first.

Then run:

```
cargo test --all
```

[WABT]: https://github.com/WebAssembly/wabt
