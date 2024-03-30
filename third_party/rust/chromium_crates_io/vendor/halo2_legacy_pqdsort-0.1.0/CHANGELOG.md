# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to Rust's notion of
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2023-03-09
### Initial release.
This crate contains a copy of the "pdqsort" (pattern-defeating quicksort) implementation
from Rust 1.56.1. This is an unstable sort, i.e. the order in the output of elements that
compare equal is not specified. In practice this order differed (for given input) between
platforms with 32-bit `usize` and those with larger (64-bit) `usize`, and may also differ
in future versions of Rust. If an application has mistakenly used unstable sorting with
non-unique elements in a context where determinism was required, this crate allows the
Rust 1.56.1 behaviour on 64-bit platforms to be replicated in other versions of Rust, and
on 32-bit platforms.

The differences from the upstream Rust standard library code are:
* to remove use of unstable or standard-library internal Rust features that would prevent
  compilation as an external crate.
* to delete unused code (`partition_at_index_loop` and `partition_at_index`).
* to add tests (based on those in the standard library) that test this crate specifically.
