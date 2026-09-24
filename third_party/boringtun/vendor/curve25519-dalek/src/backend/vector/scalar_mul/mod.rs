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

//! Implementations of various multiplication algorithms for the SIMD backends.

#[allow(missing_docs)]
pub mod variable_base;

#[allow(missing_docs)]
pub mod vartime_double_base;

#[allow(missing_docs)]
#[cfg(feature = "alloc")]
pub mod straus;

#[allow(missing_docs)]
#[cfg(feature = "alloc")]
pub mod precomputed_straus;

#[allow(missing_docs)]
#[cfg(feature = "alloc")]
pub mod pippenger;
