// -*- mode: rust; -*-
//
// This file is part of reddsa.
// Copyright (c) 2019-2021 Zcash Foundation
// See LICENSE for licensing information.
//
// Authors:
// - Deirdre Connolly <deirdre@zfnd.org>
// - Henry de Valence <hdevalence@hdevalence.ca>

use core::marker::PhantomData;

use blake2b_simd::{Params, State};

use crate::{private::SealedScalar, SigType};

/// Provides H^star, the hash-to-scalar function used by RedDSA.
pub struct HStar<T: SigType> {
    state: State,
    _marker: PhantomData<T>,
}

impl<T: SigType> Default for HStar<T> {
    fn default() -> Self {
        let state = Params::new()
            .hash_length(64)
            .personal(T::H_STAR_PERSONALIZATION)
            .to_state();
        Self {
            state,
            _marker: PhantomData::default(),
        }
    }
}

impl<T: SigType> HStar<T> {
    // Only used by FROST code
    #[allow(unused)]
    pub(crate) fn new(personalization_string: &[u8]) -> Self {
        let state = Params::new()
            .hash_length(64)
            .personal(personalization_string)
            .to_state();
        Self {
            state,
            _marker: PhantomData::default(),
        }
    }

    /// Add `data` to the hash, and return `Self` for chaining.
    pub fn update(&mut self, data: impl AsRef<[u8]>) -> &mut Self {
        self.state.update(data.as_ref());
        self
    }

    /// Consume `self` to compute the hash output.
    pub fn finalize(&self) -> T::Scalar {
        T::Scalar::from_bytes_wide(self.state.finalize().as_array())
    }
}
