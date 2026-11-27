parking_lot
============

[![Rust](https://github.com/Amanieu/parking_lot/workflows/Rust/badge.svg)](https://github.com/Amanieu/parking_lot/actions)
[![Crates.io](https://img.shields.io/crates/v/parking_lot.svg)](https://crates.io/crates/parking_lot)

[Documentation (synchronization primitives)](https://docs.rs/parking_lot/)

[Documentation (core parking lot API)](https://docs.rs/parking_lot_core/)

[Documentation (type-safe lock API)](https://docs.rs/lock_api/)

This library provides compact and efficient implementations of `Mutex`,
`RwLock`, `RecursiveRwLock`, `Condvar` and `Once`, as well as a
`ReentrantMutex` type. It also exposes a low-level API for creating your own
synchronization primitives.

## Features

Notable features of the primitives provided by this library include:

1. `Mutex` and `Once` only require 1 byte of storage space, while `Condvar`
   and `RwLock` only require 1 word of storage space. The small size of `Mutex`
   in particular encourages the use of fine-grained locks to increase
   parallelism.
2. Uncontended lock acquisition and release is done through fast inline
   paths which only require a single atomic operation.
3. Microcontention (a contended lock with a short critical section) is
   efficiently handled by spinning a few times while trying to acquire a
   lock.
4. The locks are adaptive and will suspend a thread after a few failed spin
   attempts. This makes the locks suitable for both long and short critical
   sections.
5. `RwLock` uses a task-fair locking policy which generally gives waiting
   writers priority over new readers.
6. `Condvar` is guaranteed not to produce spurious wakeups. A thread will
    only be woken up if it timed out or it was woken up by a notification.
7. `Condvar::notify_all` requeues waiters directly onto the associated `Mutex`
   and wakes them as the lock becomes available. This avoids a thundering herd
   where every thread immediately tries to acquire the mutex.
8. `RwLock` supports atomically downgrading a write lock into a read lock.
9. `Mutex` and `RwLock` allow raw unlocking without a RAII guard object.
10. `Mutex<()>` and `RwLock<()>` allow raw locking without a RAII guard
    object.
11. `Mutex` and `RwLock` support explicit fair unlocking, which hands the lock
    directly to a waiting thread.
12. `ReentrantMutex` and `RecursiveRwLock` types which support recursive
    locking.
13. An *experimental* deadlock detector that works for `Mutex`, `RwLock`,
    `RecursiveRwLock` and `ReentrantMutex`. This feature is disabled by default
    and can be enabled via the `deadlock_detection` feature.
14. `RwLock` and `RecursiveRwLock` support atomically upgrading an "upgradable"
    read lock into a write lock.
15. Optional support for [serde](https://docs.serde.rs/serde/).  Enable via the
    feature `serde`.  **NOTE!** this support is for `Mutex`, `ReentrantMutex`,
    `RwLock`, and `RecursiveRwLock` only; `Condvar` and `Once` are not currently
    supported.
16. Lock guards can be sent to other threads when the `send_guard` feature is
    enabled.

## The parking lot

To keep these primitives small, all thread queuing and suspending
functionality is offloaded to the *parking lot*. The idea behind this is
based on the WebKit [`WTF::ParkingLot`](https://webkit.org/blog/6161/locking-in-webkit/)
class, which essentially consists of a hash table mapping of lock addresses
to queues of parked (sleeping) threads. The WebKit parking lot was itself
inspired by Linux [futexes](https://man7.org/linux/man-pages/man2/futex.2.html),
but it is more powerful since it allows invoking callbacks while holding a queue
lock.

## Nightly vs stable

There are a few restrictions when using this library on stable Rust:

- The `wasm32-unknown-unknown` target is only fully supported on nightly with
  `-C target-feature=+atomics` in `RUSTFLAGS` and `-Zbuild-std=panic_abort,std`
  passed to cargo. parking_lot will work mostly fine on stable, the only
  difference is it will panic instead of block forever if you hit a deadlock.
  Just make sure not to enable `-C target-feature=+atomics` on stable as that
  will allow wasm to run with multiple threads which will completely break
  parking_lot's concurrency guarantees.

To enable nightly-only functionality, you need to enable the `nightly` feature
in Cargo (see below).

## Usage

Add this to your `Cargo.toml`:

```toml
[dependencies]
parking_lot = "0.12"
```

To enable nightly-only features, add this to your `Cargo.toml` instead:

```toml
[dependencies]
parking_lot = { version = "0.12", features = ["nightly"] }
```

The experimental deadlock detector can be enabled with the
`deadlock_detection` Cargo feature.

To allow sending `MutexGuard`s and `RwLock*Guard`s to other threads, enable the
`send_guard` option.

Note that the `deadlock_detection` and `send_guard` features are incompatible
and cannot be used together.

The core parking lot API is provided by the `parking_lot_core` crate. It is
separate from the synchronization primitives in the `parking_lot` crate so that
changes to the core API do not cause breaking changes for users of `parking_lot`.

## Minimum Rust version

The current minimum required Rust version is 1.95, but this may change at any time.

## License

Licensed under either of

 * Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE) or https://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([LICENSE-MIT](LICENSE-MIT) or https://opensource.org/licenses/MIT)

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in the work by you, as defined in the Apache-2.0 license, shall be dual licensed as above, without any
additional terms or conditions.
