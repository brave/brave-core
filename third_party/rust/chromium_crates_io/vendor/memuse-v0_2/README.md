# memuse

This crate contains traits for measuring the dynamic memory usage of Rust types.

## About

Memory-tracking is a common activity in large applications, particularly ones
that receive data from a network and store it in memory. By monitoring how much
memory is used by different areas of the application, memory pressure can be
alleviated by ignoring new packets, or implementing random drop logic for DoS
mitigation.

Measuring memory use on the stack is easy, with [`std::mem::size_of`] and
friends. Measuring memory allocated on the heap is more tricky. Applications can
use a custom global allocator to track the memory usage of different areas. This
isn't an option for reusable library code however, and the nearest alternative
(using custom allocators for individual types) is currently only an experimental
feature in nightly Rust ([`allocator_api`]).

[`std::mem::size_of`]: https://doc.rust-lang.org/stable/std/mem/fn.size_of.html
[`allocator_api`]: https://github.com/rust-lang/rust/issues/32838

This crate takes a different approach: it provides traits that library authors
can use to expose dynamic memory usage information on their types. By composing
these implementations, we gain the ability to query the amount of heap-allocated
memory in use by specific instances of types at any point in time, without any
changes to the way in which these types are constructed.

## Minimum Supported Rust Version

Rust **1.51** or newer.

In the future, we reserve the right to change MSRV (i.e. MSRV is out-of-scope
for this crate's SemVer guarantees), however when we do it will be accompanied
by a minor version bump.

## License

Licensed under either of

 * Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE) or
   http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([LICENSE-MIT](LICENSE-MIT) or http://opensource.org/licenses/MIT)

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally
submitted for inclusion in the work by you, as defined in the Apache-2.0
license, shall be dual licensed as above, without any additional terms or
conditions.

