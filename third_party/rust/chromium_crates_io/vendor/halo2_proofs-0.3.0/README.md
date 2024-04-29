# halo2_proofs [![Crates.io](https://img.shields.io/crates/v/halo2_proofs.svg)](https://crates.io/crates/halo2_proofs) #

## [Documentation](https://docs.rs/halo2_proofs)

## Minimum Supported Rust Version

Requires Rust **1.60** or higher.

Minimum supported Rust version can be changed in the future, but it will be done with a
minor version bump.

## Controlling parallelism

`halo2_proofs` currently uses [rayon](https://github.com/rayon-rs/rayon) for parallel
computation. The `RAYON_NUM_THREADS` environment variable can be used to set the number of
threads.

You can disable `rayon` by disabling the `"multicore"` feature.
Warning! Halo2 will lose access to parallelism if you disable the `"multicore"` feature.
This will significantly degrade performance.

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
