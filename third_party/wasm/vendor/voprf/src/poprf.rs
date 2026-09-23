// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

//! Contains the main POPRF API

#[cfg(feature = "alloc")]
use alloc::vec::Vec;
use core::iter::{self, Map, Repeat, Zip};

use derive_where::derive_where;
use digest::core_api::BlockSizeUser;
use digest::{Digest, Output, OutputSizeUser};
use generic_array::typenum::{IsLess, IsLessOrEqual, Unsigned, U256};
use generic_array::GenericArray;
use rand_core::{CryptoRng, RngCore};

use crate::common::{
    derive_keypair, deterministic_blind_unchecked, generate_proof, hash_to_group, i2osp_2,
    server_evaluate_hash_input, verify_proof, BlindedElement, Dst, EvaluationElement, Mode,
    PreparedEvaluationElement, Proof, STR_FINALIZE, STR_HASH_TO_SCALAR, STR_INFO,
};
#[cfg(feature = "serde")]
use crate::serialization::serde::{Element, Scalar};
use crate::{CipherSuite, Error, Group, Result};

////////////////////////////
// High-level API Structs //
// ====================== //
////////////////////////////

/// A client which engages with a [PoprfServer] in verifiable mode, meaning
/// that the OPRF outputs can be checked against a server public key.
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(Debug, Eq, Hash, Ord, PartialEq, PartialOrd; <CS::Group as Group>::Scalar, <CS::Group as Group>::Elem)]
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound = "")
)]
pub struct PoprfClient<CS: CipherSuite>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    #[cfg_attr(feature = "serde", serde(with = "Scalar::<CS::Group>"))]
    pub(crate) blind: <CS::Group as Group>::Scalar,
    #[cfg_attr(feature = "serde", serde(with = "Element::<CS::Group>"))]
    pub(crate) blinded_element: <CS::Group as Group>::Elem,
}

/// A server which engages with a [PoprfClient] in verifiable mode, meaning
/// that the OPRF outputs can be checked against a server public key.
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(Debug, Eq, Hash, Ord, PartialEq, PartialOrd; <CS::Group as Group>::Scalar, <CS::Group as Group>::Elem)]
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound = "")
)]
pub struct PoprfServer<CS: CipherSuite>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    #[cfg_attr(feature = "serde", serde(with = "Scalar::<CS::Group>"))]
    pub(crate) sk: <CS::Group as Group>::Scalar,
    #[cfg_attr(feature = "serde", serde(with = "Element::<CS::Group>"))]
    pub(crate) pk: <CS::Group as Group>::Elem,
}

/////////////////////////
// API Implementations //
// =================== //
/////////////////////////

impl<CS: CipherSuite> PoprfClient<CS>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Computes the first step for the multiplicative blinding version of
    /// DH-OPRF.
    ///
    /// # Errors
    /// [`Error::Input`] if the `input` is empty or longer than [`u16::MAX`].
    pub fn blind<R: RngCore + CryptoRng>(
        input: &[u8],
        blinding_factor_rng: &mut R,
    ) -> Result<PoprfClientBlindResult<CS>> {
        let blind = CS::Group::random_scalar(blinding_factor_rng);
        Self::deterministic_blind_unchecked_inner(input, blind)
    }

    /// Computes the first step for the multiplicative blinding version of
    /// DH-OPRF, taking a blinding factor scalar as input instead of sampling
    /// from an RNG.
    ///
    /// # Caution
    ///
    /// This should be used with caution, since it does not perform any checks
    /// on the validity of the blinding factor!
    ///
    /// # Errors
    /// [`Error::Input`] if the `input` is empty or longer than [`u16::MAX`].
    #[cfg(any(feature = "danger", test))]
    pub fn deterministic_blind_unchecked(
        input: &[u8],
        blind: <CS::Group as Group>::Scalar,
    ) -> Result<PoprfClientBlindResult<CS>> {
        Self::deterministic_blind_unchecked_inner(input, blind)
    }

    /// Can only fail with [`Error::Input`].
    fn deterministic_blind_unchecked_inner(
        input: &[u8],
        blind: <CS::Group as Group>::Scalar,
    ) -> Result<PoprfClientBlindResult<CS>> {
        let blinded_element = deterministic_blind_unchecked::<CS>(input, &blind, Mode::Poprf)?;
        Ok(PoprfClientBlindResult {
            state: Self {
                blind,
                blinded_element,
            },
            message: BlindedElement(blinded_element),
        })
    }

    /// Computes the third step for the multiplicative blinding version of
    /// DH-OPRF, in which the client unblinds the server's message.
    ///
    /// # Errors
    /// - [`Error::Info`] if the `info` is longer than `u16::MAX`.
    /// - [`Error::Input`] if the `input` is empty or longer than [`u16::MAX`].
    /// - [`Error::Protocol`] if the protocol fails and can't be completed.
    /// - [`Error::ProofVerification`] if the `proof` failed to verify.
    pub fn finalize(
        &self,
        input: &[u8],
        evaluation_element: &EvaluationElement<CS>,
        proof: &Proof<CS>,
        pk: <CS::Group as Group>::Elem,
        info: Option<&[u8]>,
    ) -> Result<Output<CS::Hash>> {
        let clients = core::array::from_ref(self);
        let messages = core::array::from_ref(evaluation_element);

        let mut batch_result =
            Self::batch_finalize(iter::once(input), clients, messages, proof, pk, info)?;
        batch_result.next().unwrap()
    }

    /// Allows for batching of the finalization of multiple [PoprfClient]
    /// and [EvaluationElement] pairs
    ///
    /// # Errors
    /// - [`Error::Info`] if the `info` is longer than `u16::MAX`.
    /// - [`Error::Protocol`] if the protocol fails and can't be completed.
    /// - [`Error::Batch`] if the number of `inputs`, `clients` and `messages`
    ///   don't match or is longer than [`u16::MAX`].
    /// - [`Error::ProofVerification`] if the `proof` failed to verify.
    ///
    /// The resulting messages can each fail individually with [`Error::Input`]
    /// if the `input` is empty or longer than [`u16::MAX`].
    pub fn batch_finalize<'a, II: 'a + Iterator<Item = &'a [u8]> + ExactSizeIterator, IC, IM>(
        inputs: II,
        clients: &'a IC,
        messages: &'a IM,
        proof: &Proof<CS>,
        pk: <CS::Group as Group>::Elem,
        info: Option<&'a [u8]>,
    ) -> Result<PoprfClientBatchFinalizeResult<'a, CS, II, IC, IM>>
    where
        CS: 'a,
        &'a IC: 'a + IntoIterator<Item = &'a PoprfClient<CS>>,
        <&'a IC as IntoIterator>::IntoIter: ExactSizeIterator,
        &'a IM: 'a + IntoIterator<Item = &'a EvaluationElement<CS>>,
        <&'a IM as IntoIterator>::IntoIter: ExactSizeIterator,
    {
        let unblinded_elements = poprf_unblind(clients, messages, pk, proof, info)?;

        finalize_after_unblind::<'a, CS, _, _>(unblinded_elements, inputs, info)
    }

    /// Only used for test functions
    #[cfg(test)]
    pub fn get_blind(&self) -> <CS::Group as Group>::Scalar {
        self.blind
    }
}

impl<CS: CipherSuite> PoprfServer<CS>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Produces a new instance of a [PoprfServer] using a supplied RNG
    ///
    /// # Errors
    /// [`Error::Protocol`] if the protocol fails and can't be completed.
    pub fn new<R: RngCore + CryptoRng>(rng: &mut R) -> Result<Self> {
        let mut seed = GenericArray::<_, <CS::Group as Group>::ScalarLen>::default();
        rng.fill_bytes(&mut seed);

        Self::new_from_seed(&seed, &[])
    }

    /// Produces a new instance of a [PoprfServer] using a supplied set of
    /// bytes to represent the server's private key
    ///
    /// # Errors
    /// [`Error::Deserialization`] if the private key is not a valid point on
    /// the group or zero.
    pub fn new_with_key(key: &[u8]) -> Result<Self> {
        let sk = CS::Group::deserialize_scalar(key)?;
        let pk = CS::Group::base_elem() * &sk;
        Ok(Self { sk, pk })
    }

    /// Produces a new instance of a [PoprfServer] using a supplied set of
    /// bytes which are used as a seed to derive the server's private key.
    ///
    /// Corresponds to DeriveKeyPair() function from the VOPRF specification.
    ///
    /// # Errors
    /// - [`Error::DeriveKeyPair`] if the `input` and `seed` together are longer
    ///   then `u16::MAX - 3`.
    /// - [`Error::Protocol`] if the protocol fails and can't be completed.
    pub fn new_from_seed(seed: &[u8], info: &[u8]) -> Result<Self> {
        let (sk, pk) = derive_keypair::<CS>(seed, info, Mode::Poprf)?;
        Ok(Self { sk, pk })
    }

    // Only used for tests
    #[cfg(test)]
    pub fn get_private_key(&self) -> <CS::Group as Group>::Scalar {
        self.sk
    }

    /// Computes the second step for the multiplicative blinding version of
    /// DH-OPRF. This message is sent from the server (who holds the OPRF key)
    /// to the client.
    ///
    /// # Errors
    /// - [`Error::Info`] if the `info` is longer than `u16::MAX`.
    /// - [`Error::Protocol`] if the protocol fails and can't be completed.
    pub fn blind_evaluate<R: RngCore + CryptoRng>(
        &self,
        rng: &mut R,
        blinded_element: &BlindedElement<CS>,
        info: Option<&[u8]>,
    ) -> Result<PoprfServerEvaluateResult<CS>> {
        let PoprfServerBatchEvaluatePrepareResult {
            mut prepared_evaluation_elements,
            prepared_tweak,
        } = self.batch_blind_evaluate_prepare(iter::once(blinded_element), info)?;

        let prepared_evaluation_element = prepared_evaluation_elements.next().unwrap();
        let prepared_evaluation_elements = core::array::from_ref(&prepared_evaluation_element);

        let PoprfServerBatchEvaluateFinishResult {
            mut messages,
            proof,
        } = Self::batch_blind_evaluate_finish(
            rng,
            iter::once(blinded_element),
            prepared_evaluation_elements,
            &prepared_tweak,
        )
        .unwrap();

        Ok(PoprfServerEvaluateResult {
            message: messages.next().unwrap(),
            proof,
        })
    }

    /// Allows for batching of the evaluation of multiple [BlindedElement]
    /// messages from a [PoprfClient]
    ///
    /// # Errors
    /// - [`Error::Info`] if the `info` is longer than `u16::MAX`.
    /// - [`Error::Protocol`] if the protocol fails and can't be completed.
    #[cfg(feature = "alloc")]
    pub fn batch_blind_evaluate<'a, R: RngCore + CryptoRng, IE>(
        &self,
        rng: &mut R,
        blinded_elements: &'a IE,
        info: Option<&[u8]>,
    ) -> Result<PoprfServerBatchEvaluateResult<CS>>
    where
        CS: 'a,
        &'a IE: 'a + IntoIterator<Item = &'a BlindedElement<CS>>,
        <&'a IE as IntoIterator>::IntoIter: ExactSizeIterator,
    {
        let PoprfServerBatchEvaluatePrepareResult {
            prepared_evaluation_elements,
            prepared_tweak,
        } = self.batch_blind_evaluate_prepare(blinded_elements.into_iter(), info)?;

        let prepared_evaluation_elements: Vec<_> = prepared_evaluation_elements.collect();

        // This can't fail because we know the size of the inputs.
        let PoprfServerBatchEvaluateFinishResult { messages, proof } =
            Self::batch_blind_evaluate_finish::<_, _, Vec<_>>(
                rng,
                blinded_elements.into_iter(),
                &prepared_evaluation_elements,
                &prepared_tweak,
            )
            .unwrap();

        let messages: Vec<_> = messages.collect();

        Ok(PoprfServerBatchEvaluateResult { messages, proof })
    }

    /// Alternative version of `batch_blind_evaluate` without
    /// memory allocation. Returned [`PreparedEvaluationElement`] have to
    /// be [`collect`](Iterator::collect)ed and passed into
    /// [`batch_blind_evaluate_finish`](Self::batch_blind_evaluate_finish).
    ///
    /// # Errors
    /// - [`Error::Info`] if the `info` is longer than `u16::MAX`.
    /// - [`Error::Protocol`] if the protocol fails and can't be completed.
    pub fn batch_blind_evaluate_prepare<'a, I: Iterator<Item = &'a BlindedElement<CS>>>(
        &self,
        blinded_elements: I,
        info: Option<&[u8]>,
    ) -> Result<PoprfServerBatchEvaluatePrepareResult<CS, I>>
    where
        CS: 'a,
    {
        let tweak = compute_tweak::<CS>(self.sk, info)?;

        Ok(PoprfServerBatchEvaluatePrepareResult {
            prepared_evaluation_elements: blinded_elements.zip(iter::repeat(tweak)).map(
                |(blinded_element, tweak)| {
                    PreparedEvaluationElement(EvaluationElement(
                        blinded_element.0 * &CS::Group::invert_scalar(tweak),
                    ))
                },
            ),
            prepared_tweak: PoprfPreparedTweak(tweak),
        })
    }

    /// See [`batch_blind_evaluate_prepare`](Self::batch_blind_evaluate_prepare)
    /// for more details.
    ///
    /// # Errors
    /// [`Error::Batch`] if the number of `blinded_elements` and
    /// `prepared_evaluation_elements` don't match or is longer then
    /// [`u16::MAX`]
    pub fn batch_blind_evaluate_finish<
        'a,
        'b,
        R: RngCore + CryptoRng,
        IB: Iterator<Item = &'a BlindedElement<CS>> + ExactSizeIterator,
        IE,
    >(
        rng: &mut R,
        blinded_elements: IB,
        prepared_evaluation_elements: &'b IE,
        prepared_tweak: &PoprfPreparedTweak<CS>,
    ) -> Result<PoprfServerBatchEvaluateFinishResult<'b, CS, IE>>
    where
        CS: 'a,
        &'b IE: IntoIterator<Item = &'b PreparedEvaluationElement<CS>>,
        <&'b IE as IntoIterator>::IntoIter: ExactSizeIterator,
    {
        let g = CS::Group::base_elem();
        let tweak = prepared_tweak.0;
        let tweaked_key = g * &tweak;

        let proof = generate_proof(
            rng,
            tweak,
            g,
            tweaked_key,
            prepared_evaluation_elements
                .into_iter()
                .map(|element| element.0 .0),
            blinded_elements.map(|element| element.0),
            Mode::Poprf,
        )?;

        let messages = prepared_evaluation_elements.into_iter().map(<fn(
            &PreparedEvaluationElement<CS>,
        ) -> _>::from(
            |element| EvaluationElement(element.0 .0),
        ));

        Ok(PoprfServerBatchEvaluateFinishResult { messages, proof })
    }

    /// Computes the output of the VOPRF on the server side
    ///
    /// # Errors
    /// [`Error::Input`]  if the `input` is longer then [`u16::MAX`].
    pub fn evaluate(
        &self,
        input: &[u8],
        info: Option<&[u8]>,
    ) -> Result<Output<<CS as CipherSuite>::Hash>> {
        let input_element = hash_to_group::<CS>(input, Mode::Poprf)?;
        if CS::Group::is_identity_elem(input_element).into() {
            return Err(Error::Input);
        };

        let tweak = compute_tweak::<CS>(self.sk, info)?;

        let evaluated_element = input_element * &CS::Group::invert_scalar(tweak);

        let issued_element = CS::Group::serialize_elem(evaluated_element);

        server_evaluate_hash_input::<CS>(input, info, issued_element)
    }

    /// Retrieves the server's public key
    pub fn get_public_key(&self) -> <CS::Group as Group>::Elem {
        self.pk
    }
}

impl<CS: CipherSuite> BlindedElement<CS>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Creates a [BlindedElement] from a raw group element.
    ///
    /// # Caution
    ///
    /// This should be used with caution, since it does not perform any checks
    /// on the validity of the value itself!
    #[cfg(feature = "danger")]
    pub fn from_value_unchecked(value: <CS::Group as Group>::Elem) -> Self {
        Self(value)
    }

    /// Exposes the internal value
    #[cfg(feature = "danger")]
    pub fn value(&self) -> <CS::Group as Group>::Elem {
        self.0
    }
}

impl<CS: CipherSuite> EvaluationElement<CS>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Creates an [EvaluationElement] from a raw group element.
    ///
    /// # Caution
    ///
    /// This should be used with caution, since it does not perform any checks
    /// on the validity of the value itself!
    #[cfg(feature = "danger")]
    pub fn from_value_unchecked(value: <CS::Group as Group>::Elem) -> Self {
        Self(value)
    }

    /// Exposes the internal value
    #[cfg(feature = "danger")]
    pub fn value(&self) -> <CS::Group as Group>::Elem {
        self.0
    }
}

/////////////////////////
// Convenience Structs //
//==================== //
/////////////////////////

/// Contains the fields that are returned by a verifiable client blind
#[derive_where(Debug; <CS::Group as Group>::Scalar, <CS::Group as Group>::Elem)]
pub struct PoprfClientBlindResult<CS: CipherSuite>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// The state to be persisted on the client
    pub state: PoprfClient<CS>,
    /// The message to send to the server
    pub message: BlindedElement<CS>,
}

/// Concrete return type for [`PoprfClient::batch_finalize`].
pub type PoprfClientBatchFinalizeResult<'a, CS, II, IC, IM> =
    FinalizeAfterUnblindResult<'a, CS, PoprfUnblindResult<'a, CS, IC, IM>, II>;

/// Contains the fields that are returned by a verifiable server evaluate
#[derive_where(Debug; <CS::Group as Group>::Scalar, <CS::Group as Group>::Elem)]
pub struct PoprfServerEvaluateResult<CS: CipherSuite>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// The message to send to the client
    pub message: EvaluationElement<CS>,
    /// The proof for the client to verify
    pub proof: Proof<CS>,
}

/// Contains the fields that are returned by a verifiable server batch evaluate
#[derive_where(Debug; <CS::Group as Group>::Scalar, <CS::Group as Group>::Elem)]
#[cfg(feature = "alloc")]
pub struct PoprfServerBatchEvaluateResult<CS: CipherSuite>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// The messages to send to the client
    pub messages: Vec<EvaluationElement<CS>>,
    /// The proof for the client to verify
    pub proof: Proof<CS>,
}

/// Concrete type of [`EvaluationElement`]s in
/// [`PoprfServerBatchEvaluatePrepareResult`].
pub type PoprfServerBatchEvaluatePreparedEvaluationElements<CS, I> = Map<
    Zip<I, Repeat<<<CS as CipherSuite>::Group as Group>::Scalar>>,
    fn(
        (
            &BlindedElement<CS>,
            <<CS as CipherSuite>::Group as Group>::Scalar,
        ),
    ) -> PreparedEvaluationElement<CS>,
>;

/// Prepared tweak by a partially verifiable server batch evaluate prepare.
#[derive_where(Clone, ZeroizeOnDrop)]
#[derive_where(Debug, Eq, Hash, Ord, PartialEq, PartialOrd; <CS::Group as Group>::Scalar)]
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize, serde::Serialize),
    serde(bound = "")
)]
pub struct PoprfPreparedTweak<CS: CipherSuite>(
    #[cfg_attr(feature = "serde", serde(with = "Scalar::<CS::Group>"))]
    <CS::Group as Group>::Scalar,
)
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>;

/// Contains the fields that are returned by a partially verifiable server batch
/// evaluate prepare
#[derive_where(Debug; I, <CS::Group as Group>::Scalar)]
pub struct PoprfServerBatchEvaluatePrepareResult<CS: CipherSuite, I>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    /// Prepared [`EvaluationElement`].
    pub prepared_evaluation_elements: PoprfServerBatchEvaluatePreparedEvaluationElements<CS, I>,
    /// Prepared tweak.
    pub prepared_tweak: PoprfPreparedTweak<CS>,
}

/// Concrete type of [`EvaluationElement`]s in
/// [`PoprfServerBatchEvaluateFinishResult`].
pub type PoprfServerBatchEvaluateFinishedMessages<'a, CS, I> = Map<
    <&'a I as IntoIterator>::IntoIter,
    fn(&PreparedEvaluationElement<CS>) -> EvaluationElement<CS>,
>;

/// Contains the fields that are returned by a verifiable server batch evaluate
/// finish.
#[derive_where(Debug; <&'a I as IntoIterator>::IntoIter, <CS::Group as Group>::Scalar)]
pub struct PoprfServerBatchEvaluateFinishResult<'a, CS: 'a + CipherSuite, I>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    &'a I: IntoIterator<Item = &'a PreparedEvaluationElement<CS>>,
{
    /// The [`EvaluationElement`]s to send to the client
    pub messages: PoprfServerBatchEvaluateFinishedMessages<'a, CS, I>,
    /// The proof for the client to verify
    pub proof: Proof<CS>,
}

/////////////////////
// Inner functions //
// =============== //
/////////////////////

/// Inner function for POPRF blind. Computes the tweaked key from the server
/// public key and info.
///
/// Can only fail with [`Error::Info`] or [`Error::Protocol`]
fn compute_tweaked_key<CS: CipherSuite>(
    pk: <CS::Group as Group>::Elem,
    info: Option<&[u8]>,
) -> Result<<CS::Group as Group>::Elem>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    // None for info is treated the same as empty bytes
    let info = info.unwrap_or_default();

    // framedInfo = "Info" || I2OSP(len(info), 2) || info
    // m = G.HashToScalar(framedInfo)
    // T = G.ScalarBaseMult(m)
    // tweakedKey = T + pkS
    // if tweakedKey == G.Identity():
    //   raise InvalidInputError
    let info_len = i2osp_2(info.len()).map_err(|_| Error::Info)?;
    let framed_info = [STR_INFO.as_slice(), &info_len, info];

    let dst = Dst::new::<CS, _, _>(STR_HASH_TO_SCALAR, Mode::Poprf);
    // This can't fail, the size of the `input` is known.
    let m = CS::Group::hash_to_scalar::<CS::Hash>(&framed_info, &dst.as_dst()).unwrap();

    let t = CS::Group::base_elem() * &m;
    let tweaked_key = t + &pk;

    // Check if resulting element
    match bool::from(CS::Group::is_identity_elem(tweaked_key)) {
        true => Err(Error::Protocol),
        false => Ok(tweaked_key),
    }
}

/// Inner function for POPRF evaluate. Computes the tweak from the server
/// private key and info.
///
/// Can only fail with [`Error::Info`] and [`Error::Protocol`].
fn compute_tweak<CS: CipherSuite>(
    sk: <CS::Group as Group>::Scalar,
    info: Option<&[u8]>,
) -> Result<<CS::Group as Group>::Scalar>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    // None for info is treated the same as empty bytes
    let info = info.unwrap_or_default();

    // framedInfo = "Info" || I2OSP(len(info), 2) || info
    // m = G.HashToScalar(framedInfo)
    // t = skS + m
    // if t == 0:
    //   raise InverseError
    let info_len = i2osp_2(info.len()).map_err(|_| Error::Info)?;
    let framed_info = [STR_INFO.as_slice(), &info_len, info];

    let dst = Dst::new::<CS, _, _>(STR_HASH_TO_SCALAR, Mode::Poprf);
    // This can't fail, the size of the `input` is known.
    let m = CS::Group::hash_to_scalar::<CS::Hash>(&framed_info, &dst.as_dst()).unwrap();

    let t = sk + &m;

    // Check if resulting element is equal to zero
    match bool::from(CS::Group::is_zero_scalar(t)) {
        true => Err(Error::Protocol),
        false => Ok(t),
    }
}

type PoprfUnblindResult<'a, CS, IC, IM> = Map<
    Zip<
        Map<
            <&'a IC as IntoIterator>::IntoIter,
            fn(&PoprfClient<CS>) -> <<CS as CipherSuite>::Group as Group>::Scalar,
        >,
        <&'a IM as IntoIterator>::IntoIter,
    >,
    fn(
        (
            <<CS as CipherSuite>::Group as Group>::Scalar,
            &'a EvaluationElement<CS>,
        ),
    ) -> <<CS as CipherSuite>::Group as Group>::Elem,
>;

/// Can only fail with [`Error::Info`], [`Error::Protocol`], [`Error::Batch] or
/// [`Error::ProofVerification`].
fn poprf_unblind<'a, CS: 'a + CipherSuite, IC, IM>(
    clients: &'a IC,
    messages: &'a IM,
    pk: <CS::Group as Group>::Elem,
    proof: &Proof<CS>,
    info: Option<&[u8]>,
) -> Result<PoprfUnblindResult<'a, CS, IC, IM>>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    &'a IC: 'a + IntoIterator<Item = &'a PoprfClient<CS>>,
    <&'a IC as IntoIterator>::IntoIter: ExactSizeIterator,
    &'a IM: 'a + IntoIterator<Item = &'a EvaluationElement<CS>>,
    <&'a IM as IntoIterator>::IntoIter: ExactSizeIterator,
{
    let info = info.unwrap_or_default();
    let tweaked_key = compute_tweaked_key::<CS>(pk, Some(info))?;

    let g = CS::Group::base_elem();

    let blinds = clients
        .into_iter()
        // Convert to `fn` pointer to make a return type possible.
        .map(<fn(&PoprfClient<CS>) -> _>::from(|x| x.blind));
    let evaluation_elements = messages.into_iter().map(|element| element.0);
    let blinded_elements = clients.into_iter().map(|client| client.blinded_element);

    verify_proof(
        g,
        tweaked_key,
        evaluation_elements,
        blinded_elements,
        proof,
        Mode::Poprf,
    )?;

    Ok(blinds
        .zip(messages)
        .map(|(blind, x)| x.0 * &CS::Group::invert_scalar(blind)))
}

type FinalizeAfterUnblindResult<'a, CS, IE, II> = Map<
    Zip<Zip<IE, II>, Repeat<&'a [u8]>>,
    fn(
        ((<<CS as CipherSuite>::Group as Group>::Elem, &[u8]), &[u8]),
    ) -> Result<GenericArray<u8, <<CS as CipherSuite>::Hash as OutputSizeUser>::OutputSize>>,
>;

/// Can only fail with [`Error::Batch`] and returned values can only fail with
/// [`Error::Info`] or [`Error::Input`] individually.
fn finalize_after_unblind<
    'a,
    CS: CipherSuite,
    IE: 'a + Iterator<Item = <CS::Group as Group>::Elem> + ExactSizeIterator,
    II: 'a + Iterator<Item = &'a [u8]> + ExactSizeIterator,
>(
    unblinded_elements: IE,
    inputs: II,
    info: Option<&'a [u8]>,
) -> Result<FinalizeAfterUnblindResult<'a, CS, IE, II>>
where
    <CS::Hash as OutputSizeUser>::OutputSize:
        IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
{
    if unblinded_elements.len() != inputs.len() {
        return Err(Error::Batch);
    }

    let info = info.unwrap_or_default();

    Ok(unblinded_elements.zip(inputs).zip(iter::repeat(info)).map(
        |((unblinded_element, input), info)| {
            let elem_len = <CS::Group as Group>::ElemLen::U16.to_be_bytes();

            // hashInput = I2OSP(len(input), 2) || input ||
            //             I2OSP(len(info), 2) || info ||
            //             I2OSP(len(unblindedElement), 2) || unblindedElement ||
            //             "Finalize"
            // return Hash(hashInput)
            let output = CS::Hash::new()
                .chain_update(i2osp_2(input.as_ref().len()).map_err(|_| Error::Input)?)
                .chain_update(input.as_ref())
                .chain_update(i2osp_2(info.as_ref().len()).map_err(|_| Error::Info)?)
                .chain_update(info.as_ref())
                .chain_update(elem_len)
                .chain_update(CS::Group::serialize_elem(unblinded_element))
                .chain_update(STR_FINALIZE)
                .finalize();

            Ok(output)
        },
    ))
}

///////////
// Tests //
// ===== //
///////////

#[cfg(test)]
mod tests {
    use core::ops::Add;
    use core::ptr;

    use generic_array::typenum::Sum;
    use generic_array::ArrayLength;
    use rand::rngs::OsRng;

    use super::*;
    use crate::common::STR_HASH_TO_GROUP;
    use crate::Group;

    fn prf<CS: CipherSuite>(
        input: &[u8],
        key: <CS::Group as Group>::Scalar,
        info: &[u8],
        mode: Mode,
    ) -> Output<CS::Hash>
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let t = compute_tweak::<CS>(key, Some(info)).unwrap();

        let dst = Dst::new::<CS, _, _>(STR_HASH_TO_GROUP, mode);
        let point = CS::Group::hash_to_curve::<CS::Hash>(&[input], &dst.as_dst()).unwrap();

        // evaluatedElement = G.ScalarInverse(t) * blindedElement
        let res = point * &CS::Group::invert_scalar(t);

        finalize_after_unblind::<CS, _, _>(iter::once(res), iter::once(input), Some(info))
            .unwrap()
            .next()
            .unwrap()
            .unwrap()
    }

    fn verifiable_retrieval<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let input = b"input";
        let info = b"info";
        let mut rng = OsRng;
        let server = PoprfServer::<CS>::new(&mut rng).unwrap();
        let client_blind_result = PoprfClient::<CS>::blind(input, &mut rng).unwrap();
        let server_result = server
            .blind_evaluate(&mut rng, &client_blind_result.message, Some(info))
            .unwrap();
        let client_finalize_result = client_blind_result
            .state
            .finalize(
                input,
                &server_result.message,
                &server_result.proof,
                server.get_public_key(),
                Some(info),
            )
            .unwrap();
        let res2 = prf::<CS>(input, server.get_private_key(), info, Mode::Poprf);
        assert_eq!(client_finalize_result, res2);
    }

    fn verifiable_bad_public_key<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let input = b"input";
        let info = b"info";
        let mut rng = OsRng;
        let server = PoprfServer::<CS>::new(&mut rng).unwrap();
        let client_blind_result = PoprfClient::<CS>::blind(input, &mut rng).unwrap();
        let server_result = server
            .blind_evaluate(&mut rng, &client_blind_result.message, Some(info))
            .unwrap();
        let wrong_pk = {
            let dst = Dst::new::<CS, _, _>(STR_HASH_TO_GROUP, Mode::Oprf);
            // Choose a group element that is unlikely to be the right public key
            CS::Group::hash_to_curve::<CS::Hash>(&[b"msg"], &dst.as_dst()).unwrap()
        };
        let client_finalize_result = client_blind_result.state.finalize(
            input,
            &server_result.message,
            &server_result.proof,
            wrong_pk,
            Some(info),
        );
        assert!(client_finalize_result.is_err());
    }

    fn verifiable_server_evaluate<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
    {
        let input = b"input";
        let info = Some(b"info".as_slice());
        let mut rng = OsRng;
        let client_blind_result = PoprfClient::<CS>::blind(input, &mut rng).unwrap();
        let server = PoprfServer::<CS>::new(&mut rng).unwrap();
        let server_result = server
            .blind_evaluate(&mut rng, &client_blind_result.message, info)
            .unwrap();

        let client_finalize = client_blind_result
            .state
            .finalize(
                input,
                &server_result.message,
                &server_result.proof,
                server.get_public_key(),
                info,
            )
            .unwrap();

        // We expect the outputs from client and server to be equal given an identical
        // input
        let server_evaluate = server.evaluate(input, info).unwrap();
        assert_eq!(client_finalize, server_evaluate);

        // We expect the outputs from client and server to be different given different
        // inputs
        let wrong_input = b"wrong input";
        let server_evaluate = server.evaluate(wrong_input, info).unwrap();
        assert!(client_finalize != server_evaluate);
    }

    fn zeroize_verifiable_client<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
        <CS::Group as Group>::ScalarLen: Add<<CS::Group as Group>::ElemLen>,
        Sum<<CS::Group as Group>::ScalarLen, <CS::Group as Group>::ElemLen>: ArrayLength<u8>,
    {
        let input = b"input";
        let mut rng = OsRng;
        let client_blind_result = PoprfClient::<CS>::blind(input, &mut rng).unwrap();

        let mut state = client_blind_result.state;
        unsafe { ptr::drop_in_place(&mut state) };
        assert!(state.serialize().iter().all(|&x| x == 0));

        let mut message = client_blind_result.message;
        unsafe { ptr::drop_in_place(&mut message) };
        assert!(message.serialize().iter().all(|&x| x == 0));
    }

    fn zeroize_verifiable_server<CS: CipherSuite>()
    where
        <CS::Hash as OutputSizeUser>::OutputSize:
            IsLess<U256> + IsLessOrEqual<<CS::Hash as BlockSizeUser>::BlockSize>,
        <CS::Group as Group>::ScalarLen: Add<<CS::Group as Group>::ElemLen>,
        Sum<<CS::Group as Group>::ScalarLen, <CS::Group as Group>::ElemLen>: ArrayLength<u8>,
        <CS::Group as Group>::ScalarLen: Add<<CS::Group as Group>::ScalarLen>,
        Sum<<CS::Group as Group>::ScalarLen, <CS::Group as Group>::ScalarLen>: ArrayLength<u8>,
    {
        let input = b"input";
        let info = b"info";
        let mut rng = OsRng;
        let server = PoprfServer::<CS>::new(&mut rng).unwrap();
        let client_blind_result = PoprfClient::<CS>::blind(input, &mut rng).unwrap();
        let server_result = server
            .blind_evaluate(&mut rng, &client_blind_result.message, Some(info))
            .unwrap();

        let mut state = server;
        unsafe { ptr::drop_in_place(&mut state) };
        assert!(state.serialize().iter().all(|&x| x == 0));

        let mut message = server_result.message;
        unsafe { ptr::drop_in_place(&mut message) };
        assert!(message.serialize().iter().all(|&x| x == 0));

        let mut proof = server_result.proof;
        unsafe { ptr::drop_in_place(&mut proof) };
        assert!(proof.serialize().iter().all(|&x| x == 0));
    }

    #[test]
    fn test_functionality() -> Result<()> {
        use p256::NistP256;
        use p384::NistP384;
        use p521::NistP521;

        #[cfg(feature = "ristretto255")]
        {
            use crate::Ristretto255;

            verifiable_retrieval::<Ristretto255>();
            verifiable_bad_public_key::<Ristretto255>();
            verifiable_server_evaluate::<Ristretto255>();

            zeroize_verifiable_client::<Ristretto255>();
            zeroize_verifiable_server::<Ristretto255>();
        }

        verifiable_retrieval::<NistP256>();
        verifiable_bad_public_key::<NistP256>();
        verifiable_server_evaluate::<NistP256>();

        zeroize_verifiable_client::<NistP256>();
        zeroize_verifiable_server::<NistP256>();

        verifiable_retrieval::<NistP384>();
        verifiable_bad_public_key::<NistP384>();
        verifiable_server_evaluate::<NistP384>();

        zeroize_verifiable_client::<NistP384>();
        zeroize_verifiable_server::<NistP384>();

        verifiable_retrieval::<NistP521>();
        verifiable_bad_public_key::<NistP521>();
        verifiable_server_evaluate::<NistP521>();

        zeroize_verifiable_client::<NistP521>();
        zeroize_verifiable_server::<NistP521>();

        Ok(())
    }
}
