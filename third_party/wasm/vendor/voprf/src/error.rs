// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

//! Errors which are produced during an execution of the protocol

use displaydoc::Display;

/// [`Result`](core::result::Result) shorthand that uses [`Error`].
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// Represents an error in the manipulation of internal cryptographic data
#[derive(Clone, Copy, Debug, Display, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub enum Error {
    /// Size of info is longer then [`u16::MAX`].
    Info,
    /// Size of input is empty or longer then [`u16::MAX`].
    Input,
    /// Size of info and seed together are longer then `u16::MAX - 3`.
    DeriveKeyPair,
    /// Failure to deserialize bytes
    Deserialization,
    /// Batched items are more then [`u16::MAX`] or length don't match.
    Batch,
    /// In verifiable mode, occurs when the proof failed to verify
    ProofVerification,
    /// The protocol has failed and can't be completed.
    Protocol,
}

/// Only used to implement [`Group`](crate::Group).
#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub enum InternalError {
    /// Size of input is empty or longer then [`u16::MAX`].
    Input,
    /// `input` is longer then [`u16::MAX`].
    I2osp,
}

#[cfg(feature = "std")]
impl std::error::Error for Error {}
