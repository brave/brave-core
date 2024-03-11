// -*- mode: rust; -*-
//
// This file is part of reddsa.
// Copyright (c) 2019-2021 Zcash Foundation
// See LICENSE for licensing information.
//
// Authors:
// - Deirdre Connolly <deirdre@zfnd.org>
// - Henry de Valence <hdevalence@hdevalence.ca>

use core::fmt;

/// An error related to RedDSA signatures.
#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum Error {
    /// The encoding of a signing key was malformed.
    MalformedSigningKey,
    /// The encoding of a verification key was malformed.
    MalformedVerificationKey,
    /// Signature verification failed.
    InvalidSignature,
}

#[cfg(feature = "std")]
impl std::error::Error for Error {}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::MalformedSigningKey => write!(f, "Malformed signing key encoding."),
            Self::MalformedVerificationKey => write!(f, "Malformed verification key encoding."),
            Self::InvalidSignature => write!(f, "Invalid signature."),
        }
    }
}
