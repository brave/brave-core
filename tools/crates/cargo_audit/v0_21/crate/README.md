# RustSec: `cargo audit`

[![Latest Version][crate-image]][crate-link]
[![Build Status][build-image]][build-link]
[![Safety Dance][safety-image]][safety-link]
![MSRV][rustc-image]
![Apache 2.0 OR MIT licensed][license-image]
[![Project Chat][chat-image]][chat-link]

Audit your dependencies for crates with security vulnerabilities reported to the
[RustSec Advisory Database].

## Requirements

`cargo audit` requires Rust **1.74** or later.

## Installation

<a href="https://repology.org/project/cargo-audit/versions"><img align="right" src="https://repology.org/badge/vertical-allrepos/cargo-audit.svg" alt="Packaging status"></a>

`cargo audit` is a Cargo subcommand and can be installed with `cargo install`:

```
$ cargo install cargo-audit --locked
```

Once installed, run `cargo audit` at the toplevel of any Cargo project.

### Alpine Linux

```
# apk add cargo-audit
```

### Arch Linux

```
# pacman -S cargo-audit
```

### MacOS

```
$ brew install cargo-audit
```

### OpenBSD

```
# pkg_add cargo-audit
```

## Screenshot

<img src="https://raw.githubusercontent.com/RustSec/cargo-audit/c857beb/img/screenshot.png" alt="Screenshot" style="max-width:100%;">

## `cargo audit fix` subcommand

This tool supports an experimental feature to automatically update `Cargo.toml`
to fix vulnerable dependency requirements.

To enable it, install `cargo audit` with the `fix` feature enabled:

```
$ cargo install cargo-audit --locked --features=fix
```

Once installed, run `cargo audit fix` to automatically fix vulnerable
dependency requirements in your `Cargo.toml`:

<img src="https://raw.githubusercontent.com/RustSec/cargo-audit/c857beb/img/screenshot-fix.png" alt="Screenshot" style="max-width:100%;">

This will modify `Cargo.toml` in place. To perform a dry run instead, which
shows a preview of what dependencies would be upgraded, run
`cargo audit fix --dry-run`.

## `cargo audit bin` subcommand

Run `cargo audit bin` followed by the paths to your binaries to audit them:

<img src="https://github.com/rustsec/rustsec/raw/46eeb09cef411bbe926a82c8a0d678a3e43299a1/.img/screenshot-bin.png" alt="Screenshot" style="max-width:100%;">

If your programs have been compiled with [`cargo auditable`](https://github.com/rust-secure-code/cargo-auditable),
the audit is fully accurate because all the necessary information is embedded in the compiled binary.

For binaries that were not compiled with [`cargo auditable`](https://github.com/rust-secure-code/cargo-auditable)
it will recover a part of the dependency list by parsing panic messages.
This will miss any embedded C code (e.g. OpenSSL) as well as roughly half of the Rust dependencies
because the Rust compiler is very good at removing unnecessary panics,
but that's better than having no vulnerability information whatsoever.

## Ignoring advisories

The first and best way to fix a vulnerability is to upgrade the vulnerable crate.

But there may be situations where an upgrade isn't available and the advisory doesn't affect your application. For example the advisory might involve a cargo feature or API that is unused.

In these cases, you can ignore advisories using the `--ignore` option.

```
$ cargo audit --ignore RUSTSEC-2017-0001
```

This option can also be configured via the [`audit.toml`](./audit.toml.example) file.

## Using `cargo audit` on Travis CI

To automatically run `cargo audit` on every build in Travis CI, you can add the following to your `.travis.yml`:

```yaml
language: rust
cache: cargo # cache cargo-audit once installed
before_script:
  - cargo install --force --locked cargo-audit
  - cargo generate-lockfile
script:
  - cargo audit
```

## Using `cargo audit` on GitHub Action

Please use [`audit-check` action](https://github.com/rustsec/audit-check) directly.

## Reporting Vulnerabilities

Report vulnerabilities by opening pull requests against the [RustSec Advisory Database]
GitHub repo:

<a href="https://github.com/RustSec/advisory-db/blob/master/CONTRIBUTING.md">
  <img alt="Report Vulnerability" width="250px" height="60px" src="https://rustsec.org/img/report-vuln-button.svg">
</a>

## License

Licensed under either of:

 * Apache License, Version 2.0 ([LICENSE-APACHE] or https://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([LICENSE-MIT] or https://opensource.org/licenses/MIT)

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in the work by you shall be dual licensed as above, without any
additional terms or conditions.

[//]: # (badges)

[crate-image]: https://img.shields.io/crates/v/cargo-audit.svg?logo=rust
[crate-link]: https://crates.io/crates/cargo-audit
[build-image]: https://github.com/RustSec/rustsec/actions/workflows/cargo-audit.yml/badge.svg
[build-link]: https://github.com/RustSec/rustsec/actions/workflows/cargo-audit.yml
[license-image]: https://img.shields.io/badge/license-Apache2.0%2FMIT-blue.svg
[rustc-image]: https://img.shields.io/badge/rustc-1.74+-blue.svg
[safety-image]: https://img.shields.io/badge/unsafe-forbidden-success.svg
[safety-link]: https://github.com/rust-secure-code/safety-dance/
[chat-image]: https://img.shields.io/badge/zulip-join_chat-blue.svg
[chat-link]: https://rust-lang.zulipchat.com/#narrow/stream/146229-wg-secure-code/

[//]: # (general links)

[RustSec Advisory Database]: https://github.com/RustSec/advisory-db/
[LICENSE-APACHE]: https://github.com/RustSec/cargo-audit/blob/main/LICENSE-APACHE
[LICENSE-MIT]: https://github.com/RustSec/cargo-audit/blob/main/LICENSE-MIT
