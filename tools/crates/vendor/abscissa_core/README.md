# ![Abscissa][logo]

[![Crate][crate-image]][crate-link]
[![Docs][docs-image]][docs-link]
[![Apache 2.0 Licensed][license-image]][license-link]
![MSRV][rustc-image]
[![Safety Dance][safety-image]][safety-link]
[![Build Status][build-image]][build-link]

Abscissa is a microframework for building Rust applications (either CLI tools
or network/web services), aiming to provide a large number of features with a
*minimal number of dependencies*, and with a *strong focus on security*.

[Documentation][docs-link]

## Features

- **command-line option parsing**: simple declarative option parser based on
  [clap]. The option parser in Abcissa contains numerous improvements which
  provide better UX and tighter integration with the other parts of the
  framework (e.g. overriding configuration settings using command-line options).
- **components**: Abscissa uses a component architecture (similar to an ECS)
  for extensibility/composability, with a minimalist implementation that still
  provides such features such as calculating dependency ordering and providing
  hooks into the application lifecycle. Newly generated apps use two components
  by default: `terminal` and `logging`.
- **configuration**: Simple parsing of TOML configurations to `serde`-parsed
  configuration types which can be dynamically updated at runtime.
- **error handling**: unified error-handling subsystem with generic error type.
- **logging**: based on the `log` to provide application-level logging.
- **secrets management**: the (optional) `secrets` module includes a `Secret`
  type which derives serde's `Deserialize` and can be used to represent secret
  values parsed from configuration files or elsewhere (e.g. credentials loaded
  from the environment or network requests)
- **terminal interactions**: support for colored terminal output (with color
  support autodetection). Useful for Cargo-like status messages with
  easy-to-use macros.

## Projects Using Abscissa

- [Canister]: deployment utility for "distroless" containers/microVMs
- [cargo-audit]: audit Cargo projects for security vulnerabilities
- [cosmon]: observability tool for Tendermint applications
- [ibc-rs]: Rust implementation of Interblockchain Communication (IBC) modules and relayer
- [rustic]: fast, encrypted, and deduplicated backups
- [Synchronicity]: distributed build system providing BFT proofs-of-reproducibility
- [Tendermint KMS]: key management system for Tendermint applications
- [Zebra]: Rust implementation of a Zcash node
- [Zerostash]: Encrypted and deduplicated backups


## Crate Structure

Abscissa presently consists of three crates:

- [abscissa]: CLI app and application generator - `cargo install abscissa`
- [abscissa_core]: main framework library
- [abscissa_derive]: custom derive support - implementation detail of `abscissa_core`
- [abscissa_tokio]: support for launching Tokio runtimes within Abscissa applications

## Minimum Supported Rust Version

Requires Rust **1.74** or newer.

## Installation

To generate a new Abscissa application, install the `abscissa` CLI utility:

```text
$ cargo install abscissa
```

## Creating a new Abscissa application

The following commands will generate an Abscissa application skeleton:

```text
$ cargo install abscissa
$ abscissa new my_cool_app
```

The resulting app is a Cargo project. The following files are particularly
noteworthy:

- `src/application.rs`: Abscissa application type for your app
- `src/commands*`: application entrypoint and subcommands. Make sure to
  check out the `start.rs` example of how to make a subcommand.
- `src/config.rs`: application configuration
- `src/error.rs`: error types

Abscissa applications are implemented as Rust libraries, but have a
`src/bin` subdirectory where the binary entrypoint lives. This means you
can run the following within your newly generated application:

```text
$ cargo run -- start world
```

This will invoke the `start` subcommand of your application (you
might want to rename that in your app) which will print the following:

```text
Hello, world!
```

You can also run the following to print basic help information:

```text
$ cargo run -- --help
 ```

## Status Macros

```text
// Print a Cargo-like justified status to STDOUT
status_ok!("Loaded", "app loaded successfully");

// Print an error message
status_err!("something bad happened");

// Print an indented attribute to STDOUT
status_attr_ok!("good", "yep");

// Print an error attribute to STDERR
status_attr_err!("error", "yep");
```

## Frequently Asked Questions (FAQ)

#### Q1: Why is it called "abscissa"?

**A1:** The word "abscissa" is the key to the [Kryptos K2] panel.

#### Q2: "Abscissa" is a hard name to remember! Got any tips?

**A2**: Imagine you're A-B testing a couple of scissors... with attitude.

## Testing Framework Changes

The main way to test framework changes is by generating an application with
Abscissa's built-in application generator and running tests against the
generated application (also rustfmt, clippy).

To generate a test application and test it automatically, you can simply do:

```text
$ cargo test
```

However, when debugging test failures against a generated app, it's helpful to
know how to drive the app generation and testing process manually. Below are
instructions on how to do so.

If you've already run:

```text
$ git clone https://github.com/iqlusioninc/abscissa/
```

...and are inside the `abscissa` directory and want to test your changes,
you can generate an application by running the following command:

```text
$ cargo run -- new /tmp/example_app --patch-crates-io='abscissa = { path = "$PWD" }'
```

This will generate a new Abscissa application in `/tmp/example_app` which
references your local copy of Abscissa.

After that, change directory to the newly generated app and run the tests
to ensure things are still working (the tests, along with rustfmt and clippy
are run as part of the CI process):

```text
$ cd /tmp/example_app # or 'pushd /tmp/example_app' and 'popd' to return
$ cargo test
$ cargo fmt -- --check # generated app is expected to pass rustfmt
$ cargo clippy
```

## Code of Conduct

We abide by the [Contributor Covenant][cc] and ask that you do as well.

For more information, please see [CODE_OF_CONDUCT.md].

## License

The **abscissa** crate is distributed under the terms of the
Apache License (Version 2.0).

Copyright © 2018-2024 iqlusion

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

<https://www.apache.org/licenses/LICENSE-2.0>

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

## Contribution

If you are interested in contributing to this repository, please make sure to
read the [CONTRIBUTING.md] and [CODE_OF_CONDUCT.md] files first.

[//]: # (badges)

[logo]: https://raw.githubusercontent.com/iqlusioninc/abscissa/main/img/abscissa.svg
[crate-image]: https://img.shields.io/crates/v/abscissa_core.svg?logo=rust
[crate-link]: https://crates.io/crates/abscissa_core
[docs-image]: https://docs.rs/abscissa_core/badge.svg
[docs-link]: https://docs.rs/abscissa_core/
[license-image]: https://img.shields.io/badge/license-Apache2.0-blue.svg
[license-link]: https://github.com/iqlusioninc/abscissa/blob/main/LICENSE
[rustc-image]: https://img.shields.io/badge/rustc-1.74+-blue.svg
[safety-image]: https://img.shields.io/badge/unsafe-forbidden-success.svg
[safety-link]: https://github.com/rust-secure-code/safety-dance/
[build-image]: https://github.com/iqlusioninc/abscissa/workflows/cli/badge.svg?branch=main&event=push
[build-link]: https://github.com/iqlusioninc/abscissa/actions

[//]: # (crate links)

[abscissa]: https://crates.io/crates/abscissa
[abscissa_core]: https://crates.io/crates/abscissa_core
[abscissa_derive]: https://crates.io/crates/abscissa_derive
[abscissa_tokio]: https://crates.io/crates/abscissa_tokio

[//]: # (general links)

[cargo]: https://github.com/rust-lang/cargo
[cargo features]: https://doc.rust-lang.org/cargo/reference/manifest.html#the-features-section
[Kryptos K2]: https://en.wikipedia.org/wiki/Kryptos#Solution_of_passage_2
[cc]: https://contributor-covenant.org
[CODE_OF_CONDUCT.md]: https://github.com/iqlusioninc/abscissa/blob/main/CODE_OF_CONDUCT.md
[CONTRIBUTING.md]: https://github.com/iqlusioninc/abscissa/blob/main/CONTRIBUTING.md

[//]: # (projects using abscissa)

[Tendermint KMS]: https://github.com/iqlusioninc/tmkms
[Canister]: https://github.com/iqlusioninc/canister
[cargo-audit]: https://github.com/rustsec/cargo-audit
[cosmon]: https://github.com/iqlusioninc/cosmon
[ibc-rs]: https://github.com/informalsystems/ibc-rs
[rustic]: https://github.com/rustic-rs/rustic
[Synchronicity]: https://github.com/iqlusioninc/synchronicity
[Zebra]: https://github.com/ZcashFoundation/zebra
[Zerostash]: https://github.com/rsdy/zerostash
