// -*- mode: rust; -*-
//
// This file is part of curve25519-dalek.
// Copyright (c) 2016-2021 isis lovecruft
// Copyright (c) 2016-2019 Henry de Valence
// See LICENSE for licensing information.
//
// Authors:
// - isis agora lovecruft <isis@patternsinthevoid.net>
// - Henry de Valence <hdevalence@hdevalence.ca>

#![no_std]
#![cfg_attr(
    all(
        curve25519_dalek_backend = "simd",
        nightly,
        any(target_arch = "x86", target_arch = "x86_64")
    ),
    feature(stdarch_x86_avx512)
)]
#![cfg_attr(
    all(curve25519_dalek_backend = "simd", nightly),
    feature(avx512_target_feature)
)]
#![cfg_attr(docsrs, feature(doc_auto_cfg, doc_cfg, doc_cfg_hide))]
#![cfg_attr(docsrs, doc(cfg_hide(docsrs)))]
//------------------------------------------------------------------------
// Documentation:
//------------------------------------------------------------------------
#![doc(
    html_logo_url = "https://cdn.jsdelivr.net/gh/dalek-cryptography/curve25519-dalek/docs/assets/dalek-logo-clear.png"
)]
#![doc = include_str!("../README.md")]
//------------------------------------------------------------------------
// Linting:
//------------------------------------------------------------------------
#![cfg_attr(allow_unused_unsafe, allow(unused_unsafe))]
#![warn(
    clippy::unwrap_used,
    missing_docs,
    rust_2018_idioms,
    unused_lifetimes,
    unused_qualifications
)]
// Requires MSRV 1.77 as it does not allow build.rs gating
#![allow(unexpected_cfgs)]

//------------------------------------------------------------------------
// External dependencies:
//------------------------------------------------------------------------

#[cfg(feature = "alloc")]
#[allow(unused_imports)]
#[macro_use]
extern crate alloc;

// TODO: move std-dependent tests to `tests/`
#[cfg(test)]
#[macro_use]
extern crate std;

#[cfg(feature = "digest")]
pub use digest;

// Internal macros. Must come first!
#[macro_use]
pub(crate) mod macros;

//------------------------------------------------------------------------
// curve25519-dalek public modules
//------------------------------------------------------------------------

// Scalar arithmetic mod l = 2^252 + ..., the order of the Ristretto group
pub mod scalar;

// Point operations on the Montgomery form of Curve25519
pub mod montgomery;

// Point operations on the Edwards form of Curve25519
pub mod edwards;

// Group operations on the Ristretto group
pub mod ristretto;

// Useful constants, like the Ed25519 basepoint
pub mod constants;

// External (and internal) traits.
pub mod traits;

//------------------------------------------------------------------------
// curve25519-dalek internal modules
//------------------------------------------------------------------------

// Finite field arithmetic mod p = 2^255 - 19
pub(crate) mod field;

// Arithmetic backends (using u32, u64, etc) live here
#[cfg(docsrs)]
pub mod backend;
#[cfg(not(docsrs))]
pub(crate) mod backend;

// Generic code for window lookups
pub(crate) mod window;

pub use crate::{
    edwards::EdwardsPoint, montgomery::MontgomeryPoint, ristretto::RistrettoPoint, scalar::Scalar,
};

// Build time diagnostics for validation
#[cfg(curve25519_dalek_diagnostics = "build")]
mod diagnostics;
