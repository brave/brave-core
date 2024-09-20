// -*- mode: rust; -*-
//
// This file is part of ed25519-dalek.
// Copyright (c) 2017-2019 isis lovecruft
// See LICENSE for licensing information.
//
// Authors:
// - isis agora lovecruft <isis@patternsinthevoid.net>

//! Errors which may occur when parsing keys and/or signatures to or from wire formats.

// rustc seems to think the typenames in match statements (e.g. in
// Display) should be snake cased, for some reason.
#![allow(non_snake_case)]

use core::fmt;
use core::fmt::Display;

#[cfg(feature = "std")]
use std::error::Error;

/// Internal errors.  Most application-level developers will likely not
/// need to pay any attention to these.
#[derive(Clone, Copy, Debug, Eq, PartialEq, Hash)]
pub(crate) enum InternalError {
    PointDecompression,
    ScalarFormat,
    /// An error in the length of bytes handed to a constructor.
    ///
    /// To use this, pass a string specifying the `name` of the type which is
    /// returning the error, and the `length` in bytes which its constructor
    /// expects.
    BytesLength {
        name: &'static str,
        length: usize,
    },
    /// The verification equation wasn't satisfied
    Verify,
    /// Two arrays did not match in size, making the called signature
    /// verification method impossible.
    #[cfg(feature = "batch")]
    ArrayLength {
        name_a: &'static str,
        length_a: usize,
        name_b: &'static str,
        length_b: usize,
        name_c: &'static str,
        length_c: usize,
    },
    /// An ed25519ph signature can only take up to 255 octets of context.
    #[cfg(feature = "digest")]
    PrehashedContextLength,
    /// A mismatched (public, secret) key pair.
    MismatchedKeypair,
}

impl Display for InternalError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            InternalError::PointDecompression => write!(f, "Cannot decompress Edwards point"),
            InternalError::ScalarFormat => write!(f, "Cannot use scalar with high-bit set"),
            InternalError::BytesLength { name: n, length: l } => {
                write!(f, "{} must be {} bytes in length", n, l)
            }
            InternalError::Verify => write!(f, "Verification equation was not satisfied"),
            #[cfg(feature = "batch")]
            InternalError::ArrayLength {
                name_a: na,
                length_a: la,
                name_b: nb,
                length_b: lb,
                name_c: nc,
                length_c: lc,
            } => write!(
                f,
                "Arrays must be the same length: {} has length {},
                              {} has length {}, {} has length {}.",
                na, la, nb, lb, nc, lc
            ),
            #[cfg(feature = "digest")]
            InternalError::PrehashedContextLength => write!(
                f,
                "An ed25519ph signature can only take up to 255 octets of context"
            ),
            InternalError::MismatchedKeypair => write!(f, "Mismatched Keypair detected"),
        }
    }
}

#[cfg(feature = "std")]
impl Error for InternalError {}

/// Errors which may occur while processing signatures and keypairs.
///
/// This error may arise due to:
///
/// * Being given bytes with a length different to what was expected.
///
/// * A problem decompressing `r`, a curve point, in the `Signature`, or the
///   curve point for a `PublicKey`.
///
/// * A problem with the format of `s`, a scalar, in the `Signature`.  This
///   is only raised if the high-bit of the scalar was set.  (Scalars must
///   only be constructed from 255-bit integers.)
///
/// * Failure of a signature to satisfy the verification equation.
pub type SignatureError = ed25519::signature::Error;

impl From<InternalError> for SignatureError {
    #[cfg(not(feature = "std"))]
    fn from(_err: InternalError) -> SignatureError {
        SignatureError::new()
    }

    #[cfg(feature = "std")]
    fn from(err: InternalError) -> SignatureError {
        SignatureError::from_source(err)
    }
}
