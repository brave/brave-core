// -*- mode: rust; -*-
//
// This file is part of reddsa.
// Copyright (c) 2019-2021 Zcash Foundation
// See LICENSE for licensing information.
//
// Authors:
// - Henry de Valence <hdevalence@hdevalence.ca>
// - Conrado Gouvea <conradoplg@gmail.com>

//! RedDSA Signatures
use core::{fmt, marker::PhantomData};

use crate::{hex_if_possible, SigType};

/// A RedDSA signature.
#[derive(Copy, Clone, Eq, PartialEq)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub struct Signature<T: SigType> {
    pub(crate) r_bytes: [u8; 32],
    pub(crate) s_bytes: [u8; 32],
    pub(crate) _marker: PhantomData<T>,
}

impl<T: SigType> fmt::Debug for Signature<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Signature")
            .field("r_bytes", &hex_if_possible(&self.r_bytes))
            .field("s_bytes", &hex_if_possible(&self.s_bytes))
            .finish()
    }
}

impl<T: SigType> From<[u8; 64]> for Signature<T> {
    fn from(bytes: [u8; 64]) -> Signature<T> {
        let mut r_bytes = [0; 32];
        r_bytes.copy_from_slice(&bytes[0..32]);
        let mut s_bytes = [0; 32];
        s_bytes.copy_from_slice(&bytes[32..64]);
        Signature {
            r_bytes,
            s_bytes,
            _marker: PhantomData,
        }
    }
}

impl<T: SigType> From<Signature<T>> for [u8; 64] {
    fn from(sig: Signature<T>) -> [u8; 64] {
        let mut bytes = [0; 64];
        bytes[0..32].copy_from_slice(&sig.r_bytes[..]);
        bytes[32..64].copy_from_slice(&sig.s_bytes[..]);
        bytes
    }
}
