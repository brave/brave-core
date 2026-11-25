# Constellation

Rust library implementing the *Constellation* threshold aggregation
mechanism. It allows clients to submit ordered, granular data at
the highest that is possible whilst maintaining crowd-based
anonymity. The receiving server can only decode messages whose
contents were also submitted by some threshold number of other
clients, blocking identification of unique behaviour.

Constellation is a _nested_ version of the [STAR](https://arxiv.org/abs/2109.10074)
protocol and this library makes use of the [sta-rs](https://github.com/brave/sta-rs)
Rust implementation.

## Disclaimer

WARNING this library has not been audited, use at your own risk! This
code is under active development and may change substantially in future
versions.

## Quickstart

Build & test:
```
cargo build
cargo test
```
