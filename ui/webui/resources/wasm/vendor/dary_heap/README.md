# dary_heap

[![CI](https://github.com/hanmertens/dary_heap/workflows/CI/badge.svg)](https://github.com/hanmertens/dary_heap/actions?query=workflow%3ACI+branch%3Amaster)
[![Crates.io](https://img.shields.io/crates/v/dary_heap.svg)](https://crates.io/crates/dary_heap)
[![Docs.rs](https://docs.rs/dary_heap/badge.svg)](https://docs.rs/dary_heap)

Rust implementation of a [*d*-ary heap][wiki]. The *d* = 2 version is present in
the standard library as [`BinaryHeap`][std-binaryheap], but using a higher value
for *d* can bring performance improvements in many use cases. This is because a
higher arity *d* (maximum number of children each node can have) means the heap
contains less layers, making adding elements to the heap faster. However,
removing elements is slower, because then the amount of work per layer is higher
as there are more children. The latter effect is often diminished due to higher
cache locality. Therefore, overall performance is often increased if *d* > 2 but
not too high. Benchmarking is necessary to determine the best value of *d* for a
specific use case.

## Compatibility and stability

The API of this crate aims to be analogous to that of [`BinaryHeap` in the
standard library][std-binaryheap]. Feature-gated API in the standard library is
also feature-gated in `dary_heap`, see [the section on features](#features) for
more information. In fact, the code in `dary_heap` is directly based on that of
the standard library. The `BinaryHeap` provided by this crate should therefore
provide similar performance as that of the standard library, and the other heap
types provided by this crate may provide performance improvements.

The version of the standard library this crate is based on is currently 1.89.0.
The aim is to keep the crate in sync with the latest stable Rust release.

The minimum supported Rust version (MSRV) is currently 1.51.0. The MSRV can be
increased in a minor level release, but not in a patch level release. If a low
MSRV is not required, the `extra` feature can be enabled to add features and
performance enhancements which require a newer version of Rust (currently
1.61.0), which can change in a patch level release.

There are no stability guarantees for the `unstable` and `unstable_nightly`
features. Changes to the behavior of nullary heaps (that should not be used
anyway) are also not considered to be breaking and can happen in a patch level
release.

## Features

- `extra`: add features that require a higher MSRV (currently 1.61.0).
  - add `shrink_to` method to shrink heap capacity to a lower bound.
  - add `try_reserve` method to try to reserve additional capacity in the heap.
  - add `try_reserve_exact` method to try to reserve minimal additonal capacity.
  - make `new` method `const`.
  - make `PeekMut::pop` potentially faster.
- `serde`: add support for (de)serialization using [Serde][serde].
- `unstable`: enable support for experimental (unstable) features:
  - add `drain_sorted` method which is like `drain` but yields elements in heap
    order.
  - add `into_iter_sorted` method which is like `into_iter` but yields elements
    in heap order.
  - add `PeekMut::refresh` method to obtain the current greatest element of the
    heap after modification of the previously greatest element.
- `unstable_nightly`: enable support for experimental (unstable) features that
  require a nightly Rust compiler:
  - implement methods defined by unstable feature `exact_size_is_empty` on
    `ExactSizeIterator`s in this crate.
  - implement methods defined by unstable feature `extend_one`.
  - implement `SourceIter` and `InPlaceIterable` for `IntoIter`.
  - implement `TrustedLen` for iterators if possible (only when `unstable` is
    also enabled).

## License

`dary_heap` is licensed under either of

 * Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE) or
   https://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([LICENSE-MIT](LICENSE-MIT) or
   https://opensource.org/licenses/MIT)

at your option.

[wiki]: https://en.wikipedia.org/wiki/D-ary_heap
[std-binaryheap]: https://doc.rust-lang.org/std/collections/struct.BinaryHeap.html
[serde]: https://serde.rs
