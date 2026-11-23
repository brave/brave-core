// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

//! Unstable digest algorithms for use with the [`crate::digest`] module.
//!
//! [`KECCAK256`] is used with [`crate::digest::digest`] and
//! [`crate::digest::Context`] exactly like the stable algorithms. This module is
//! unavailable with the `fips` feature.

pub use crate::digest::sha::{KECCAK256, KECCAK256_OUTPUT_LEN};
