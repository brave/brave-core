//! # Migrating from `nom`
//!
//! For comparisons with `nom`, see
//! - [Why `winnow`][super::why]
//! - [parse-rosetta-rs](https://github.com/rosetta-rs/parse-rosetta-rs/)
//!
//! What approach you take depends on the size and complexity of your parser.
//! For small, simple parsers, its likely easiest to directly port from `nom`.
//! When trying to look for the equivalent of a `nom` combinator, search in the docs for the name
//! of the `nom` combinator.  It is expected that, where names diverge, a doc alias exists.
//! See also the [List of combinators][crate::combinator].
//!
//! For larger parsers, it is likely best to take smaller steps
//! - Easier to debug when something goes wrong
//! - Deprecation messages will help assist through the process
//!
//! The workflow goes something like:
//! 1. Run `cargo rm nom && cargo add winnow@0.3`
//! 1. Ensure everything compiles and tests pass, ignoring deprecation messages (see [migration
//!    notes](https://github.com/winnow-rs/winnow/blob/v0.3-main/CHANGELOG.md#nom-migration-guide))
//! 1. Commit
//! 1. Switch any `impl FnMut(I) -> IResult<I, O, E>` to `impl Parser<I, O, E>`
//! 1. Resolve deprecation messages
//! 1. Commit
//! 1. Run `cargo add winnow@0.4`
//! 1. Ensure everything compiles and tests pass, ignoring deprecation messages (see [changelog](https://github.com/winnow-rs/winnow/blob/v0.4-main/CHANGELOG.md#compatibility-2) for more details)
//! 1. Commit
//! 1. Resolve deprecation messages
//! 1. Commit
//! 1. Run `cargo add winnow@0.5`
//! 1. Ensure everything compiles and tests pass, ignoring deprecation messages (see [migration
//!     notes](https://github.com/winnow-rs/winnow/blob/v0.5.0/CHANGELOG.md))
//! 1. Commit
//! 1. Resolve deprecation messages
//! 1. Commit
//!
//! For example migrations, see
//! - [git-config-env](https://github.com/gitext-rs/git-config-env/pull/11) (nom to winnow 0.3)
//! - [git-conventional](https://github.com/crate-ci/git-conventional/pull/37) (nom to winnow 0.3,
//!   adds explicit tracing for easier debugging)
//! - [typos](https://github.com/crate-ci/typos/pull/664) (nom to winnow 0.3)
//! - [cargo-smart-release](https://github.com/Byron/gitoxide/pull/948) (gradual migration from nom
//!   to winnow 0.5)
//! - [gix-config](https://github.com/Byron/gitoxide/pull/951) (gradual migration from nom
//!   to winnow 0.5)
//! - [gix-protocol](https://github.com/Byron/gitoxide/pull/1009) (gradual migration from nom
//!   to winnow 0.5)
//! - [gitoxide](https://github.com/Byron/gitoxide/pull/956) (gradual migration from nom
//!   to winnow 0.5)
